/** @file utils.cc
 * @brief General utility functions.
 */
/* Copyright (c) 2010 Richard Boulton
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
#include "utils.h"

#include "str.h"
#include <string>
#include <string.h>

std::string
get_sys_error(int errno_value)
{
#ifdef HAVE_STRERROR_R
#define ERRBUFSIZE 256
    char buf[ERRBUFSIZE];
    buf[0] = '\0';
#ifdef STRERROR_R_CHAR_P
    return std::string(strerror_r(errno_value, buf, ERRBUFSIZE));
#else
    int ret = strerror_r(errno_value, buf, ERRBUFSIZE);
    if (ret == 0) {
	buf[ERRBUFSIZE - 1] = '\0';
	return std::string(buf);
    } else {
	return std::string("code " + str(errno_value));
    }
#endif
#undef ERRBUFSIZE
#else
    return std::string(strerror(errno_value));
#endif
}
