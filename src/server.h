/** @file server.h
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

#ifndef XAPSRV_INCLUDED_SERVER_H
#define XAPSRV_INCLUDED_SERVER_H

#include "logger.h"
#include "settings.h"

class Server {
    /// The settings used by this server.
    const ServerSettings & settings;

    /// Logger to use.
    Logger logger;

    /// Flag, set to true when the server has started.
    bool started;

    /// Flag, set to true when a shutdown request has been made.
    bool shutting_down;

    /** A pipe, written to when a request to shutdown is made to wake up the
     *  main thread.
     */
    int shutdown_pipe;

    /** The error message, when the server has failed to start or terminated
     *  on error.
     */
    std::string error_message;

    /** Set the error message to describe a system error.
     */
    void set_sys_error(const std::string & message, int errno_value);

    /** Set up the signal handlers.
     */
    bool set_up_signal_handlers();

  public:
    Server(const ServerSettings & settings_);
    ~Server();

    /** Run the server.
     *
     *  This should usually only be called once - if called repeatedly,
     *  subsequent calls will return immediately.
     *
     *  @retval true if the server ran without error and terminated cleanly.
     *  @retval false if the server failed to start, or terminated on error.  In this situation, a message describing the error can be obtained with
     */
    bool run();

    /** Start the shutdown of the server.
     *
     *  This returns immediately, but the server may take some time to shut
     *  down fully.
     *
     *  It is safe to call this from a signal handler, and from any thread.  It
     *  is safe to call it repeatedly - only the first call will have any
     *  effect.  If the server hasn't started yet, it will exit immediately
     *  when started.
     *
     *  @retval false if already shutting down
     *  @retval true if not already shutting down
     */
    bool shutdown();

    /** Get the error message.
     *
     *  This will contain a message when the server has failed to start or
     *  terminated on error.  Otherwise, it will be empty.
     */
    const std::string & get_error_message() const { return error_message; }
};

#endif /* XAPSRV_INCLUDED_SERVER_H */
