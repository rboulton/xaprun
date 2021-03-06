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
#include "json/json.h"
#include "server/serverinternal.h"
#include "server/worker.h"
#include "server/workerpool.h"
#include "str.h"
#include "utils.h"
#include "xappy/indexerworker.h"
#include "xappy/searchworker.h"

// Maximum length of a message length.  9 = 10^9 bytes, and since we pull the
// whole message into memory to deal with it, we almost certainly don't want to
// allow this to be any larger.
#define MAX_MSG_LEN_LEN 9

void
XappyDispatcher::send_fatal_error(int connection_num,
				  const std::string & payload)
{
    Json::FastWriter writer;
    Json::Value root;
    root[Json::StaticString("ok")] = 0;
    root[Json::StaticString("msg")] = payload;
    send_response(connection_num, str(payload.size() + 1) + " F" + writer.write(root));
    // FIXME - close the connection after sending this response.
}

void
XappyDispatcher::send_error_response(const Message & msg,
				     const std::string & payload)
{
    Json::FastWriter writer;
    Json::Value root;
    root[Json::StaticString("ok")] = 0;
    root[Json::StaticString("msg")] = payload;
    send_msg_response(msg.connection_num, msg.msgid, 'E', writer.write(root));
}

void
XappyDispatcher::send_msg_response(int connection_num,
				   const std::string & msgid,
				   char status,
				   const std::string & payload)
{
    logger->error(std::string("Sending response to msgid: '") + msgid + "'");
    std::string buf(" ");
    buf += msgid;
    buf += " ";
    buf += status;
    buf += payload;
    send_response(connection_num, str(buf.size() - 1) + buf);
}

Worker *
XappyDispatcher::get_worker(const std::string & group, int current_workers)
{
    (void) current_workers;
    if (group == "search") {
    	return new SearchWorker();
    }
    if (startswith(group, "indexer")) {
    	return new IndexerWorker(group.substr(7));
    }
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
	send_fatal_error(msg.connection_num, "Invalid message");
	return false;
    }
    size_t j = buf.find(' ', i + 1);
    if (j >= end) {
	logger->error("Invalid message: no payload");
	send_fatal_error(msg.connection_num, "Invalid message");
	return false;
    }

    msg.msgid = buf.substr(pos, i - pos);
    msg.target = buf.substr(i + 1, j - (i + 1));
    msg.payload = buf.substr(j + 1, end - (j + 1));

    return true;
}

static void
split_target(const std::string & target,
	     std::vector<std::string> & components) {
    std::string::size_type i = 0, j;
    while (true) {
	j = target.find('/', i);
	if (j == target.npos) {
	    components.push_back(target.substr(i));
	    return;
	}
	components.push_back(target.substr(i, j - i));
	i = j + 1;
    }
}

/** Route a message appropriately.
 */
void
XappyDispatcher::route_message(int connection_num,
			       const std::string & buf, size_t pos, size_t msglen)
{
    Message msg(connection_num);
    if (!build_message(msg, buf, pos, msglen)) return;

    if (msg.target.empty()) {
	logger->error("Invalid message: empty target");
	send_error_response(msg, "Invalid message");
	return;
    }
    std::string target = msg.target.substr(1);
    std::vector<std::string> components;
    split_target(target, components);
    switch (msg.target[0]) {
	case 'G': // GET
	    {
		if (target == "version") {
		    send_msg_response(connection_num, msg.msgid, 'S', VERSION);
		    return;
		}
		if (components.size() >= 2 && components[0] == "db") {
		    logger->info("Got request on db '" + components[1] + "'");
		    send_to_worker("search", msg);
		    return;
		}
	    }
	    break;
	case 'P': // POST
	    {
	    }
	    break;
	case 'U': // PUT
	    {
		if (components.size() >= 2 && components[0] == "db") {
		    logger->info("Got request on db '" + components[1] + "'");
		    send_to_worker("indexer_" + components[1], msg);
		    return;
		}
	    }
	    break;
	case 'D': // DELETE
	    {
	    }
	    break;
	default:
	    logger->error(std::string("Unknown message type: '") + msg.target[0] + "'");
	    send_error_response(msg, "Invalid message");
	    return;
    }
    send_error_response(msg, "Not found");
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
		logger->error("Resyncing - skipping " + str(pos - startpos) +
			      " characters: \"" +
			      buf.substr(startpos, pos - startpos) + "\"");
		send_fatal_error(connection_num, "Invalid message");
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
