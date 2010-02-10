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

#include "settings.h"

class Server {
  public:
    class Internal;
    /** @internal Internal state.
     *
     *  These are exposed to avoid needing to friend other internal classes.
     */
    Internal * internal;

    /** Create a server.
     *
     *  This does not start the server - call run() for that.
     *
     *  @param settings The settings for the server.
     */
    Server(const ServerSettings & settings);

    /** Clean up any outstanding server resources.
     */
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

    /** Get the error message.
     *
     *  This will contain a message when the server has failed to start or
     *  terminated on error.  Otherwise, it will be empty.
     */
    const std::string & get_error_message() const;
};

#endif /* XAPSRV_INCLUDED_SERVER_H */
