/** @file io_wrappers.h
 * @brief Convenient wrappers around unix IO system calls.
 */
/* Copyright 2010 Richard Boulton
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

#ifndef XAPSRV_INCLUDED_IO_WRAPPERS_H
#define XAPSRV_INCLUDED_IO_WRAPPERS_H

#include <string>

/** Open a file for appending, creating it if not present.
 *
 *  @param filename The filename to open.
 *
 *  @returns the file descriptor, or -1 if unable to open.  Errno will be set
 *  if -1 is returned.
 */
int io_open_append_create(const char * filename);

/** Write to a file descriptor.
 *
 *  @param fd The file descriptor to write to.
 *  @param data The data to write.
 *  @param len The length (in bytes) of the data to write.
 *
 *  @returns true if written successfully, false otherwise.  Errno will be set
 *  if false is returned.
 */
bool io_write(int fd, const char * data, ssize_t len);

/** Close a file descriptor.
 *
 *  @returns true if closed successfully, false otherwise.  Errno will be set
 *  if false is returned.
 */
bool io_close(int fd);

#define CHUNKSIZE 4096

/** Read an exact number of bytes, blocking until all the bytes are read.
 *
 *  @param result A string which will be set to contain the bytes which have
 *  been read.  If this is shorter than the number of bytes requested, EOF
 *  was encountered.
 *  @param fd The file descriptor to read from.
 *  @param to_read The number of bytes to read.
 *
 *  @returns true if read without error (though possibly having been unable
 *  to read all bytes requested due to EOF), false otherwise.  Errno will be
 *  set if false is returned.
 */
bool io_read_exact(std::string & result, int fd, size_t to_read);

#endif /* XAPSRV_INCLUDED_IO_WRAPPERS_H */