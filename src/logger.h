/** @file logger.h
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

#ifndef XAPSRV_INCLUDED_LOGGER_H
#define XAPSRV_INCLUDED_LOGGER_H

#include <pthread.h>
#include <string>

class Logger {
    /// The filename of the log.
    std::string filename;

    /// The filedescriptor to log to, or -1 if not open.
    mutable int logfd;

  public:
    /// Make a new logger, to log to the specifed filename.
    Logger(const std::string & filename_);

    /// Destructor.
    ~Logger();

    /// Make a log entry.
    void log(char type, const std::string & msg);

    /// Make a log entry for an error, including information from errno.
    void syserr(const std::string & msg);

    /// Make a log entry for a fatal error.
    void fatal(const std::string & msg);

    /// Make a log entry giving general information about the server status.
    void info(const std::string & msg);
};

#endif /* XAPSRV_INCLUDED_LOGGER_H */
