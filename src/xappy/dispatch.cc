/** @file dispatch.cc
 * @brief Dispatch requests to workers.
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
#include "dispatch.h"

#include <algorithm>
#include <ctype.h>
#include "server/serverinternal.h"
#include "server/worker.h"
#include "server/workerpool.h"
#include "str.h"
#include "xappy/searchworker.h"

// Maximum length of a message length.  9 = 10^9 bytes, and since we pull the
// whole message into memory to deal with it, we almost certainly don't want to
// allow this to be any larger.
// FIXME - make this configurable.
#define MAX_MSG_LEN_LEN 9

Worker *
XappyDispatcher::get_worker(const std::string & group, int current_workers)
{
    (void) current_workers;
    if (group == "search") {
    	return new SearchWorker();
    }
    return new SearchWorker();
    return NULL;
}

bool
XappyDispatcher::build_message(Message & msg,
			       const std::string & buf, size_t pos, size_t msglen)
{
    size_t end = pos + msglen;
    size_t i = buf.find(' ', pos);

    if (i >= end) {
	logger->error("Invalid message: no target or payload");
	return false;
    }
    size_t j = buf.find(' ', i + 1);
    if (j >= end) {
	logger->error("Invalid message: no payload");
	return false;
    }

    msg.msgid = buf.substr(pos, i - pos);
    msg.target = buf.substr(i + 1, j - (i + 1));
    msg.payload = buf.substr(j + 1, end - (j + 1));

    return true;
}

/** Route a message appropriately.
 */
void
XappyDispatcher::route_message(int connection_num,
			       const std::string & buf, size_t pos, size_t msglen)
{
    Message msg(connection_num);
    if (!build_message(msg, buf, pos, msglen))
	return;

    send_to_worker("echo", msg);
}

bool
XappyDispatcher::dispatch_request(int connection_num, std::string & buf)
{
    std::string::size_type pos = 0;
    std::string::size_type size = buf.size();
    std::string::size_type startpos = 0;
    bool found = false;

    while(true) {
	// Ignore whitespace between messages.
	while (pos < size && isspace(buf[pos])) {
	    ++pos;
	}
	startpos = pos;

	// Read the message length, in decimal
	int msglen = 0;
	size_t max_msg_len_len = MAX_MSG_LEN_LEN;
	if (size < MAX_MSG_LEN_LEN) 
	    max_msg_len_len = size;
	while (pos < max_msg_len_len && isdigit(buf[pos])) {
	    msglen = msglen * 10 + (buf[pos] - '0');
	    ++pos;
	}

	// Move past the space
	if (pos >= size)
	    break;
	if (buf[pos] != ' ') {
	    pos = buf.find_first_of("\n\r", pos, 2);
	    if (pos != buf.npos) {
		logger->debug("Resyncing - skipping " + str(pos - startpos) +
			      " characters: \"" +
			      buf.substr(startpos, pos - startpos) + "\"");
		startpos = pos;
		continue;
	    } else {
		break;
	    }
	}
	++pos;

	// Check if we've got the whole message now.
	if (pos + msglen > size)
	    break;

	route_message(connection_num, buf, pos, msglen);
	found = true;
	pos += msglen;
    }

    if (startpos != 0) {
	logger->debug("Dealt with " + str(startpos) + " characters in buffer, " +
		      str(size - startpos) + " characters left");
	buf.erase(0, startpos);
    } else {
	logger->debug(str(size) + " characters in buffer");
    }
    return found;
}
