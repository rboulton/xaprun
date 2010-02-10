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

#include <errno.h>
#include "io_wrappers.h"
#include <pthread.h>
#include "signals.h"
#include "settings.h"
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "utils.h"

Server::Server(const ServerSettings & settings)
	: internal(new Server::Internal(settings))
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


Server::Internal::Internal(const ServerSettings & settings_)
	: settings(settings_),
	  logger(settings_.log_filename),
	  started(false),
	  shutting_down(false),
	  shutdown_pipe_write_end(-1),
	  shutdown_pipe_read_end(-1),
	  error_message()
{
}

Server::Internal::~Internal()
{
}

bool
Server::Internal::run()
{
    if (started || shutting_down) {
	// Exit immediately, without an error.
	return true;
    }
    started = true;

    // Create the socket used for signalling a shutdown request.
    {
	int fds[2];
	int ret = socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, fds);
	if (ret == -1) {
	    set_sys_error("Couldn't create internal socketpair", errno);
	    return false;
	}
	shutdown_pipe_write_end = fds[0];
	shutdown_pipe_read_end = fds[1];
    }
    set_up_signal_handlers(this);
    try {
	if (start_listening()) {
	    mainloop();
	    stop_listening();
	}

	release_signal_handlers();
	if (!io_close(shutdown_pipe_write_end)) {
	    set_sys_error("Couldn't close internal socketpair", errno);
	}
	if (!io_close(shutdown_pipe_read_end)) {
	    set_sys_error("Couldn't close internal socketpair", errno);
	}
    } catch(...) {
	release_signal_handlers();
	(void)io_close(shutdown_pipe_write_end);
	(void)io_close(shutdown_pipe_read_end);
	throw;
    }
    return error_message.empty();
}

void
Server::Internal::mainloop()
{
    while (true) {
	int maxfd = 0;
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(shutdown_pipe_read_end, &rfds);
	if (shutdown_pipe_read_end > maxfd)
	    maxfd = shutdown_pipe_read_end;

	int ret = select(maxfd + 1, &rfds, NULL, NULL, NULL);
	if (ret == -1) {
	    if (errno == EINTR) continue;
	    set_sys_error("Select failed", errno);
	    return;
	}
	if (ret == 0) {
	    continue;
	}

	if (FD_ISSET(shutdown_pipe_read_end, &rfds)) {
	    // Listen for a byte on the shutdown pipe.
	    std::string result;
	    if (!io_read_exact(result, shutdown_pipe_read_end, 1)) {
		set_sys_error("Couldn't read from internal socket", errno);
	    }
	    return;
	}
    }
}

void
Server::Internal::shutdown()
{
    (void) io_write(shutdown_pipe_write_end, "S");
}

void
Server::Internal::emergency_shutdown()
{
}

void
Server::Internal::set_sys_error(const std::string & message, int errno_value)
{
    if (!error_message.empty()) {
	// Don't overwrite an error condition set earlier
	return;
    }
    error_message = message + ": " + get_sys_error(errno_value);
}

#if 0
/** Thread start-point for mainloop thread. */
static void
run_main_thread(void * arg_ptr)
{
    Server::Internal * arg = reinterpret_cast<Server::Internal *>(arg_ptr);
    arg->mainloop();
}
#endif

bool
Server::Internal::start_listening()
{
#if 0
    // Start the main thread.
    {
	ret = pthread_create(&main_thread, NULL, run_main_thread, this);
	if (ret == -1) {
	    set_sys_error("Couldn't create main thread", errno);
	    return false;
	}
    }
#endif

    return true;
}

void
Server::Internal::stop_listening()
{

}
