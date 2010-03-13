/** @file dispatch.h
 * @brief Dispatch requests for xappy-server
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

#ifndef XAPSRV_INCLUDED_DISPATCH_H
#define XAPSRV_INCLUDED_DISPATCH_H

#include "server/server.h"

class XappyDispatcher : public Dispatcher {
  public:
    bool dispatch_request(int connection_num, std::string & buf);
    Worker * get_worker(const std::string & group, int current_workers);

    void send_error_response(int connection_num, const std::string & msg);
    void send_msg_response(int connection_num, const std::string & msgid,
			   char status, const std::string & msg);

    bool build_message(Message & msg,
		  const std::string & buf, size_t pos, size_t msglen);
    void route_message(int connection_num,
		       const std::string & buf, size_t pos, size_t msglen);
};

#endif /* XAPSRV_INCLUDED_DISPATCH_H */
