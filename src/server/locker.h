/** @file locker.h
 * @brief Convenient wrapper for holding thread locks.
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

#ifndef XAPSRV_INCLUDED_LOCKER_H
#define XAPSRV_INCLUDED_LOCKER_H

#include <pthread.h>

/** A simple wrapper around a mutex.
 */
class Locker {
    pthread_mutex_t mutex;
  public:
    Locker() {
	pthread_mutex_init(&mutex, NULL);
    }
    ~Locker() {
	pthread_mutex_destroy(&mutex);
    }
    void lock() {
	pthread_mutex_lock(&mutex);
    }
    void unlock() {
	pthread_mutex_unlock(&mutex);
    }
};

/** Hold a lock while this object is in context.
 */
class ContextLocker {
    Locker & locker;
    bool locked;
  public:
    ContextLocker(Locker & locker_) : locker(locker_), locked(true) {
	lock();
    }
    ~ContextLocker() {
	unlock();
    }
    void lock() {
	if (locked)
	    return;
	locker.lock();
	locked = true;
    }
    void unlock() {
	if (!locked)
	    return;
	locker.unlock();
	locked = false;
    }
};

#endif /* XAPSRV_INCLUDED_IO_LOCKER_H */
