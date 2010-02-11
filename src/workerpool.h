/** @file workerpool.h
 * @brief Worker pools
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

#ifndef XAPSRV_INCLUDED_WORKERPOOL_H
#define XAPSRV_INCLUDED_WORKERPOOL_H

#include "logger.h"
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

// Forward declaration
class Worker;

class WorkerFactory
{
  public:
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

/** Details about a worker.
 */
struct WorkerDetails {
    /** The group that the worker is in.
     */
    std::string group;

    /** The number of outstanding messages that the worker has been sent.
     */
    int messages;

    /** True iff the worker has no significant outstanding work to do before
     *  exiting.
     */
    bool ready_to_exit;

    /** Create a new WorkerDetails object.
     *
     *  @param group_ The group that the worker is in.
     */
    WorkerDetails(const std::string & group_)
	    : group(group_), messages(0), ready_to_exit(true) {}
};

class WorkerPool {
  public:
    Logger * logger;

  private:
    WorkerFactory * factory;

    /** Mutex which must be held when accessing the list of workers.
     */
    pthread_mutex_t workerlist_mutex;

    /** For each current worker, the group that it is in, and the number of
     *  outstanding messages it has been sent.
     */
    std::map<Worker *, WorkerDetails> workers;

    /** All the current workers, in each group.
     *
     *  The set of workers in this structure is identical to the set in
     *  `workers`.
     */
    std::map<std::string, std::set<Worker *> > workers_by_group;

    /** Workers which have been asked to stop.
     */
    std::set<Worker *> exiting_workers;

    /** Workers which have exited.
     *
     *  These need to be joined and then deleted.
     */
    std::queue<Worker *> exited_workers;

    /** Add a new worker.
     *
     *  workerlist_mutex must be held when this is called.
     */
    void add_worker(Worker * worker, const std::string & group);

    /** Remove a worker from the list of current workers.
     *
     *  workerlist_mutex must be held when this is called.
     *
     *  @returns true if the worker was in the list of current workers, false
     *  otherwise.
     */
    bool remove_current_worker(Worker * worker);

    /** Request that a worker stops.
     *
     *  workerlist_mutex must be held when this is called.
     */
    void request_exit(Worker * worker);

    /** Send a message to a worker in a specified group.
     *
     *  workerlist_mutex must be held when this is called.
     *
     *  Creates the worker if necessary.
     */
    void do_send_to_worker(const std::string & group,
			   int connection_num,
			   const std::string & msg);
  public:
    WorkerPool(Logger * logger_);
    ~WorkerPool();

    /** Set the factory to use to make workers.
     */
    void set_factory(WorkerFactory * factory_);

    /** Called by a worker to indicate that it has handled one message.
     *
     *  @param ready_to_exit true if the worker has no significant outstanding
     *  work to do (other than messages which it has not yet handled).
     *
     *  The worker's mutex must not be held when this is called.
     */
    void worker_message_handled(Worker * worker, bool ready_to_exit);

    /** Called by a worker to indicate that it has exited.
     *
     *  The worker's mutex must not be held when this is called.
     */
    void worker_exited(Worker * worker);

    /** Try to send a message to a worker, creating one if needed.
     *
     *  If the worker can't be created currently, the message will be queued
     *  and passed to a worker later.
     */
    void send_to_worker(const std::string & group,
			int connection_num,
			const std::string & msg);

#if 0 
    /** Cancel any queued messages for a given connection.
     */
    void cancel_queued_messages(int connection_num);
#endif
};

#endif /* XAPSRV_INCLUDED_WORKERPOOL_H */
