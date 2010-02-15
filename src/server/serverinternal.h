/** @file serverinternal.h
 * @brief Server internal implementation
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

#ifndef XAPSRV_INCLUDED_SERVERINTERNAL_H
#define XAPSRV_INCLUDED_SERVERINTERNAL_H

#include "logger.h"
#include <queue>
#include "settings.h"
#include <map>
#include "workerpool.h"

class Dispatcher;

struct Connection {
    int read_fd;
    int write_fd;
    std::string read_buf;
    std::string write_buf;

    Connection()
	    : read_fd(-1), write_fd(-1)
    {}

    Connection(int read_fd_, int write_fd_)
	    : read_fd(read_fd_), write_fd(write_fd_)
    {}
};

class ServerInternal {
    /// The settings used by this server.
    const ServerSettings & settings;

    /// The dispatcher used by this server.
    Dispatcher * dispatcher;

    /// Logger to use.
    Logger logger;

    /// Flag, set to true when the server has started.
    bool started;

    /// Flag, set to true when a shutdown request has been made.
    bool shutting_down;

    /** The write end of a pipe, written to when a request to wake up the
     *  main thread is made.
     */
    int nudge_write_end;

    /** The read end of a pipe, written to when a request to wake up the
     *  main thread is made.
     */
    int nudge_read_end;

    /** The error message, when the server has failed to start or terminated
     *  on error.
     */
    std::string error_message;

    /** The connections to listen on and write responses to.
     *
     *  The connection numbers must be >= 0.
     */
    std::map<int, Connection> connections;

    /** The current workers.
     */
    WorkerPool workers;

    /** Mutex to be held whenever accessing outgoing_messages.
     */
    pthread_mutex_t outgoing_message_mutex;

    /** Messages ready to be passed to a connection.
     */
    std::queue<std::pair<int, std::string> > outgoing_messages;

    /** Run the main loop.
     */
    void mainloop();

    /** Start the server listening.
     */
    bool start_listening();

    /** Stop the server listening.
     *
     *  Waits until all active connections are closed before returning.
     */
    void stop_listening();

    /** Dispatch all responses which are ready.
     */
    bool dispatch_responses();

  public:
    ServerInternal(const ServerSettings & settings_, Dispatcher * dispatcher_);
    ~ServerInternal();

    /** Start up and run the server.
     */
    bool run();

    /** Start the shutdown of the server.
     *
     *  This returns immediately, but the server may take some time to shut
     *  down fully.
     *
     *  It is safe to call this from any thread, or from a signal handler.  It
     *  is safe to call it repeatedly - only the first call will have any
     *  effect.
     *
     *  This must not be called until the server has started.
     */
    void shutdown();

    /** Perform an emergency shutdown of the server.
     *
     *  This is intended to clean up the server just before a forced quit.
     *
     *  This should simply unlink any temporary files currently opened by
     *  the server.  Any errors when unlinking may be ignored.
     *
     *  This is safe to call this from a signal handler.
     */
    void emergency_shutdown();

    /** Set the error message to describe a system error.
     */
    void set_sys_error(const std::string & message, int errno_value);

    /** Get the error message.
     */
    const std::string & get_error_message() const { return error_message; }

    /** Queue a response for sending back to the server.
     */
    void queue_response(int connection_num, const std::string & response);
};

#endif /* XAPSRV_INCLUDED_SERVERINTERNAL_H */
