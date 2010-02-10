/** @file worker.h
 * @brief Base class for workers.
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

#ifndef XAPSRV_INCLUDED_WORKER_H
#define XAPSRV_INCLUDED_WORKER_H

#include <pthread.h>
#include "serverinternal.h"
#include <string>
#include <queue>
#include <vector>

/** Exception raised to stop a worker.
 */
class StopWorkerException {
    std::string message;
  public:
    StopWorkerException() : message() {}
    StopWorkerException(const std::string & message_) : message(message_) {}
    const std::string & get_message() const { return message; }
};

class Worker {
    /// The server internals controling this worker.
    Server::Internal * server;

    /** The messages received.
     */
    std::queue<std::string> messages;

    /** Flag, set to true when a stop has been requested.
     */
    bool stop_requested;

    /** Flag, set to true when worker is started.
     */
    bool started;

    /** The thread containing the worker.
     */
    pthread_t worker_thread;

    /** Condition used to signal receipt of new messages.
     */
    pthread_cond_t message_cond;

    /** Mutex to be held when using message_cond.
     */
    pthread_mutex_t message_mutex;

  protected:
    /** Wait for a message to be received.
     *
     *  If stop() is called on the worker, this may raise a
     *  StopWorkerException, which must be allowed to pass through the caller
     */
    std::string wait_for_message();

  public:
    /** Create a new worker.
     */
    Worker(Server::Internal * server_);

    /** Clean up after the worker.
     */
    ~Worker();

    /** Start the worker running (in a new thread).
     */
    bool start();

    /** Stop the worker running.
     *
     *  This returns immediately, but signals the worker to stop.
     */
    void stop();

    /** Wait for the worker to stop running.
     */
    void join();

    /** Send a message to the worker.
     */
    void send_message(const std::string & msg);

    /** Main method of the worker.
     *
     *  This should call wait_for_message() to get messages, 
     */
    virtual void run() = 0;

    /** Cleanup the worker.
     *
     *  This will be called after the stop() method has been called, to give
     *  the worker a chance to clean up after itself.
     */
    virtual void cleanup();
};

#endif /* XAPSRV_INCLUDED_WORKER_H */
