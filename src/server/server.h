/** @file server.h
 * @brief An asynchronous, worker based, server.
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

class Logger;
class ServerInternal;
class WorkerPool;
class WorkerThread;

struct Message {
    int connection_num;
    std::string msgid;
    std::string target;
    std::string payload;
    Message() {}
    Message(int connection_num_)
	    : connection_num(connection_num_)
    {}
};

class Worker {
    WorkerThread * thread;
  protected:

    /** Wait for a message to be received.
     *
     *  If stop() is called on the worker, this may raise a
     *  StopWorkerException, which must be allowed to pass through the caller
     *
     *  @returns a message indicating the next task to do.  Interpreting this
     *  message is entirely up to the subclass, with the exception that a
     *  message with a negative connection number is a request for the worker
     *  to clean up after itself - the next call to wait_for_message() after an
     *  empty message should have the `ready_to_exit` parameter set to True.
     *
     *  @param ready_to_exit True if the worker has no significant work to do
     *  before exiting.  The worker's cleanup() method will still be called
     *  before the worker exits (except in the case of an unclean emergency
     *  shutdown of the server).
     */
    Message wait_for_message(bool ready_to_exit);

    /** Send a response to a connection.
     */
    void send_response(int connection_num, const std::string & msg);

  public:
    /** @internal
     *
     *  Set the thread that this worker is run in.
     */
    void set_thread(WorkerThread * thread_) { thread = thread_; }

    /** Main method of the worker.
     *
     *  This should call wait_for_message() to get messages, 
     */
    virtual void run() = 0;

    /** Cleanup the worker.
     *
     *  This will be called after the stop() method has been called, to give
     *  the worker a final chance to clean up after itself.
     *
     *  The default implementation does nothing.
     */
    virtual void cleanup();
};

class Dispatcher {
  private:
    WorkerPool * pool;

  protected:
    Logger * logger;
    ServerInternal * server;
    friend class ServerInternal;

    void send_to_worker(const std::string & group, const Message & msg);
    void send_response(int connection_num, const std::string & msg);

  public:
    /** Pull the first request from the start of "buf", and dispatch it.
     *
     *  Modifies "buf" to remove the request.
     *
     *  @retval true if a request was found in "buf", false otherwise.
     */
    virtual bool dispatch_request(int connection_num, std::string & buf) = 0;

    /** Get a newly allocated worker for the given group.
     *
     *  This may return NULL if there are already the maximum number of workers
     *  for that group.  In this case, an attempt to make the worker will be
     *  made again later.
     *
     *  @param group The group that the worker is for.
     *  @param current_workers The number of workers currently in that group.
     *
     *  @retval A new worker, or NULL if worker couldn't be allocated currently.
     */

    virtual Worker * get_worker(const std::string & group,
				int current_workers) = 0;
};

class Server {
  public:
    /** @internal Internal state.
     *
     *  These are exposed to avoid needing to friend other internal classes.
     */
    ServerInternal * internal;

    /** Create a server.
     *
     *  This does not start the server - call run() for that.
     *
     *  @param settings The settings for the server.
     */
    Server(const ServerSettings & settings, Dispatcher * dispatcher);

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
