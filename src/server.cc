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

#include <errno.h>
#include "io_wrappers.h"
#include "settings.h"
#include <sys/socket.h>
#include <sys/types.h>
#include "utils.h"

Server::Server(const ServerSettings & settings_)
	: settings(settings_),
	  logger(settings_.log_filename),
	  started(false),
	  shutting_down(false),
	  shutdown_pipe(-1)
{
}

Server::~Server()
{
    shutdown();
}

bool
Server::run()
{
    if (started || shutting_down) {
	// Exit immediately, without an error.
	return true;
    }
    started = true;

    // Create the socket used for signalling a shutdown request.
    int shutdown_pipe_listen_end;
    {
	int fds[2];
	int ret = socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, fds);
	if (ret == -1) {
	    set_sys_error("Couldn't create internal socketpair", errno);
	    return false;
	}
	shutdown_pipe = fds[0];
	shutdown_pipe_listen_end = fds[1];
    }
    set_up_signal_handlers();

    // Start the worker threads.


    // Listen for a byte on the shutdown pipe.
    std::string result;
    if (!io_read_exact(result, shutdown_pipe_listen_end, 1)) {
	set_sys_error("Couldn't read from internal socket", errno);
	return false;
    }
    
    // Shut down the worker threads.


    return true;
}

bool
Server::shutdown()
{
    if (shutting_down) return false;
    // Note - this method must be safe to call inside a signal handler.
    // Therefore, all it does is set the "closing down" flag, and send a byte
    // on the "closing down" pipe.
    shutting_down = true;
    if (started) {
	io_write(shutdown_pipe, "S");
    }
    return true;
}

void
Server::set_sys_error(const std::string & message, int errno_value)
{
    if (!error_message.empty()) {
	// Don't overwrite an error condition set earlier
	return;
    }
    error_message = message + ": " + get_sys_error(errno_value);
}
