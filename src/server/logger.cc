/** @file logger.cc
 * @brief Logging code
 */
/* Copyright 2009 Richard Boulton
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

// Includes: config, this file's header, then local headers, then system.
#include <config.h>
#include "logger.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include "io_wrappers.h"
#include "str.h"
#include <sys/types.h>
#include <unistd.h>

Logger::Logger(const std::string & filename_)
	: filename(filename_),
	  logfd(-1)
{
}

Logger::~Logger()
{
    if (logfd == -1) return;
    if (!io_close(logfd)) {
	fprintf(stderr, "Can't close log file at %s: %s\n",
		filename.c_str(), strerror(errno));
    }
}

void
Logger::log(char type, const std::string & msg)
{
    if (logfd == -1) {
	if (filename.empty()) return;
	logfd = io_open_append_create(filename.c_str());
	if (logfd == -1) {
	    fprintf(stderr, "Can't open log file at %s: %s\n",
		   filename.c_str(), strerror(errno));
	    return;
	}
    }

    std::string line(1, type);
    line.append(str(getpid()));
    line.append(".", 2);
    line.append(str(pthread_self()));
    line.append(": ", 2);
    line.append(msg);
    line.append(1, '\n');
    if (!io_write(logfd, line.data(), line.size())) {
	fprintf(stderr, "Can't write to log file at %s: %s\n",
		filename.c_str(), strerror(errno));
    }
}

void
Logger::syserr(const std::string & msg)
{
    log('S', msg + ": " + strerror(errno));
}

void
Logger::error(const std::string & msg)
{
    log('E', msg);
}
void
Logger::info(const std::string & msg)
{
    log('I', msg);
}
