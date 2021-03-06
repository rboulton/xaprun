/** @file server.cc
 * @brief Implementation of a server around Xapian
 */
/*
 * Copyright (c) 2010 Richard Boulton
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <config.h>
#include "server.h"
#include "serverinternal.h"

#include <algorithm>
#include <assert.h>
#include <errno.h>
#include "io_wrappers.h"
#include <pthread.h>
#include "signals.h"
#include "settings.h"
#include "str.h"
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "utils.h"
#include "worker.h"
#include "workerpool.h"

void
Dispatcher::send_to_worker(const std::string & group, const Message & msg)
{
    pool->send_to_worker(group, msg);
}

void
Dispatcher::send_response(int connection_num, const std::string & msg)
{
    server->queue_response(connection_num, msg);
}

Server::Server(const ServerSettings & settings, Dispatcher * dispatcher)
	: internal(new ServerInternal(settings, dispatcher))
{
}

Server::~Server()
{
    delete internal;
}

bool
Server::run()
{
    return internal->run();
}

const std::string &
Server::get_error_message() const
{
    return internal->get_error_message();
}


ServerInternal::ServerInternal(const ServerSettings & settings_, Dispatcher * dispatcher_)
	: settings(settings_),
	  dispatcher(dispatcher_),
	  logger(settings_.log_filename),
	  started(false),
	  shutting_down(false),
	  nudge_write_end(-1),
	  nudge_read_end(-1),
	  error_message(),
	  workers(&logger, dispatcher_, this)
{
    dispatcher->server = this;
    dispatcher->pool = &workers;
    dispatcher->logger = &logger;
    pthread_mutex_init(&outgoing_message_mutex, NULL);
}

ServerInternal::~ServerInternal()
{
    pthread_mutex_destroy(&outgoing_message_mutex);
}

bool
ServerInternal::run()
{
    if (started || shutting_down) {
	// Exit immediately, without an error.
	return true;
    }
    started = true;
    logger.info("Starting server");

    // Create the socket used for signalling a shutdown request.
    {
	int fds[2];
	int ret = socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, fds);
	if (ret == -1) {
	    set_sys_error("Couldn't create internal socketpair", errno);
	    return false;
	}
	nudge_write_end = fds[0];
	nudge_read_end = fds[1];
    }
    set_up_signal_handlers(this);
    try {
	if (start_listening()) {
	    mainloop();
	    stop_listening();
	    workers.stop();
	    workers.join();
	}

	release_signal_handlers();
	if (!io_close(nudge_write_end)) {
	    set_sys_error("Couldn't close internal socketpair", errno);
	}
	if (!io_close(nudge_read_end)) {
	    set_sys_error("Couldn't close internal socketpair", errno);
	}
    } catch(...) {
	release_signal_handlers();
	(void)io_close(nudge_write_end);
	(void)io_close(nudge_read_end);
	throw;
    }
    logger.info("Shut down");
    return error_message.empty();
}

void
ServerInternal::mainloop()
{
    while (!connections.empty()) {
	int maxfd = 0;
	fd_set rfds;
	fd_set wfds;
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_SET(nudge_read_end, &rfds);
	if (nudge_read_end > maxfd)
	    maxfd = nudge_read_end;

	// Mark all the filedescriptors that Connections are interested in.
	std::map<int, Connection>::iterator i;
	for (i = connections.begin(); i != connections.end(); ++i) {
	    FD_SET(i->second.read_fd, &rfds);
	    if (i->second.read_fd > maxfd)
		maxfd = i->second.read_fd;
	    if (!i->second.write_buf.empty()) {
		FD_SET(i->second.write_fd, &wfds);
		if (i->second.write_fd > maxfd)
		    maxfd = i->second.write_fd;
	    }
	}

	// Wait for one of the filedescriptors to be ready
	int ret = select(maxfd + 1, &rfds, &wfds, NULL, NULL);
	if (ret == -1) {
	    if (errno == EINTR) continue;
	    set_sys_error("Select failed", errno);
	    return;
	}
	if (ret == 0) {
	    continue;
	}

	// Check the nudge pipe
	if (FD_ISSET(nudge_read_end, &rfds)) {
	    std::string result;
	    if (!io_read_append(result, nudge_read_end)) {
		set_sys_error("Couldn't read from internal socket", errno);
		return;
	    }
	    if (result.find('S') != result.npos) {
		// Shutdown
		logger.info("Shutting down");
		return;
	    }
	    // Currently, the only other thing we can get is 'R', indicating
	    // that there are responses to deliver.
	    if (!dispatch_responses())
		return;
	}

	// Check each connection's file descriptors.
	std::set<int> closed_connections;
	for (i = connections.begin(); i != connections.end(); ++i) {
	    if (FD_ISSET(i->second.read_fd, &rfds)) {
		int bytes_read = io_read_append(i->second.read_buf,
						i->second.read_fd, 65536);
		if (bytes_read < 0) {
		    logger.syserr("Failed to read from fd " +
				  str(i->second.read_fd) +
				  " for connection " +
				  str(i->first));
		} else if (bytes_read == 0) {
		    logger.info("Connection " + str(i->first) + " closed");
		    closed_connections.insert(i->first);
		} else {
		    // Dispatch all the requests in the buffer.
		    while (dispatcher->dispatch_request(i->first,
							i->second.read_buf)) {}
		}
	    }
	    if (FD_ISSET(i->second.write_fd, &wfds)) {
		int written = io_write_some(i->second.write_fd, i->second.write_buf);
		if (written <= 0) {
		    logger.syserr("Failed to write to fd " +
				  str(i->second.write_fd) +
				  " for connection " +
				  str(i->first));
		    closed_connections.insert(i->first);
		} else {
		    assert((size_t)written <= i->second.write_buf.size());
		    i->second.write_buf.erase(0, written);
		}
	    }
	}
	std::set<int>::iterator j;
	for (j = closed_connections.begin();
	     j != closed_connections.end(); ++j) {
	    i = connections.find(*j);
	    if (i != connections.end()) {
		connections.erase(i);
	    }
	}
    }
}

void
ServerInternal::shutdown()
{
    logger.info("Received shutdown request");
    (void) io_write(nudge_write_end, "S");
}

void
ServerInternal::emergency_shutdown()
{
    logger.info("Performing emergency shutdown");
}

void
ServerInternal::set_sys_error(const std::string & message, int errno_value)
{
    std::string new_message = message + ": " + get_sys_error(errno_value);
    logger.error(new_message);
    if (!error_message.empty()) {
	// Don't overwrite an error condition set earlier
	return;
    }
    error_message = new_message;
}

bool
ServerInternal::start_listening()
{
    if (settings.use_stdio) {
	logger.info("Listening on stdio");
	connections[0] = Connection(0, 1);
    }

    // FIXME - listen on tcp interface specified in settings

    return true;
}

void
ServerInternal::stop_listening()
{
    // FIXME - stop listening on tcp interface
}

bool
ServerInternal::dispatch_responses()
{
    if (pthread_mutex_lock(&outgoing_message_mutex) != 0) {
	set_sys_error("Couldn't lock outgoing message mutex", errno);
	return false;
    }
    try {
	while (!outgoing_messages.empty()) {
	    int conn_num = outgoing_messages.front().first;
	    std::map<int, Connection>::iterator i = connections.find(conn_num);
	    if (i != connections.end()) {
		logger.debug("Dispatching response for connection " +
			    str(conn_num));
		i->second.write_buf.append(outgoing_messages.front().second);
	    } else {
		// log the inability to send the messsage
		logger.info("Couldn't add response to connection number " +
			    str(conn_num) + " - connection not found");
	    }
	    outgoing_messages.pop();
	}
    } catch(...) {
	pthread_mutex_unlock(&outgoing_message_mutex);
	throw;
    }
    if (pthread_mutex_unlock(&outgoing_message_mutex) != 0) {
	set_sys_error("Couldn't unlock outgoing message mutex", errno);
	return false;
    }
    return true;
}

void
ServerInternal::queue_response(int connection_num,
				 const std::string & response)
{
    if (pthread_mutex_lock(&outgoing_message_mutex) != 0) {
	throw StopWorkerException("Couldn't lock outgoing message mutex");
    }
    try {
	outgoing_messages.push(make_pair(connection_num, response));
    } catch(...) {
	pthread_mutex_unlock(&outgoing_message_mutex);
	throw;
    }
    if (pthread_mutex_unlock(&outgoing_message_mutex) != 0) {
	throw StopWorkerException("Couldn't unlock outgoing message mutex");
    }
    (void) io_write(nudge_write_end, "R");
}
