/** @file worker.cc
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

#include <config.h>
#include "workerpool.h"

#include <assert.h>
#include "server.h"
#include "str.h"
#include "worker.h"

void
WorkerPool::add_worker(WorkerThread * worker, const std::string & group)
{
    try {
	workers.insert(std::make_pair(worker, WorkerDetails(group)));
    } catch(...) {
	delete worker;
	throw;
    }

    // Add the worker to `workers_by_group`
    std::map<std::string, std::set<WorkerThread *> >::iterator i;
    i = workers_by_group.find(group);
    if (i == workers_by_group.end()) {
	std::pair<std::map<std::string, std::set<WorkerThread *> >::iterator,
		bool> j;
	j = workers_by_group.insert(
		std::make_pair(group, std::set<WorkerThread *>()));
	i = j.first;
	assert(j.second);
    }
    i->second.insert(worker);
}

bool
WorkerPool::remove_current_worker(WorkerThread * worker)
{
    std::map<WorkerThread *, WorkerDetails>::iterator i;
    i = workers.find(worker);
    if (i == workers.end())
	return false;
    std::map<std::string, std::set<WorkerThread *> >::iterator j;
    j = workers_by_group.find(i->second.group);
    if (j == workers_by_group.end())
	return false;

    std::set<WorkerThread *>::iterator k = j->second.find(worker);
    if (k == j->second.end())
	return false;
    j->second.erase(k);
    workers.erase(i);
    return true;
}

void
WorkerPool::request_exit(WorkerThread * worker)
{
    logger->debug("Sending stop request to worker");
    if (!remove_current_worker(worker)) {
	logger->error("Couldn't remove worker - not in list of current "
		      "workers.  Possible resource leak.");
    }
    try {
	exiting_workers.insert(worker);
    } catch(...) {
	logger->error("Couldn't add worker to list of exiting workers. "
		      "Possible resource leak.");
	throw;
    }
    worker->stop();
}

void
WorkerPool::send_to_worker(const std::string & group,
			   const Message & msg)
{
    ContextLocker lock(workerlist_mutex);
    // Look for a worker with no messages waiting.
    std::map<WorkerThread *, WorkerDetails>::iterator k;
    WorkerThread * workerthread = NULL;

    std::map<std::string, std::set<WorkerThread *> >::iterator i;
    i = workers_by_group.find(group);
    if (i != workers_by_group.end()) {
	std::set<WorkerThread *>::iterator j;
	for (j = i->second.begin(); j != i->second.end(); ++j) {
	    k = workers.find(*j);
	    assert(k != workers.end());
	    if (k->second.messages == 0) {
		workerthread = k->first;
		break;
	    }
	}
    }
    if (!workerthread) {
	// FIXME - queue the message if can't, or shouldn't, make the
	// worker.
	logger->debug("Starting new worker");
	Worker * worker = dispatcher->get_worker(group, 0);
	workerthread = new WorkerThread(server, this, worker);
	worker->set_thread(workerthread);
	add_worker(workerthread, group);
	k = workers.find(workerthread);
	workerthread->start();
    }
    logger->debug("sending request from connection " + str(msg.connection_num) +
		 " to worker");
    ++(k->second.messages);
    k->second.ready_to_exit = false;
    workerthread->send_message(msg);
}

WorkerPool::WorkerPool(Logger * logger_, Dispatcher * dispatcher_,
		       ServerInternal * server_)
	: logger(logger_), dispatcher(dispatcher_), server(server_)
{
}

WorkerPool::~WorkerPool()
{
    {
	// Cleanup current workers.
	std::map<WorkerThread *, WorkerDetails>::iterator i;
	for (i = workers.begin(); i != workers.end(); ++i) {
	    i->first->stop();
	}
	for (i = workers.begin(); i != workers.end(); ++i) {
	    i->first->join();
	    delete i->first;
	}
    }
    {
	// Cleanup exiting workers.
	std::set<WorkerThread *>::iterator i;
	for (i = exiting_workers.begin(); i != exiting_workers.end(); ++i) {
	    (*i)->join();
	    delete *i;
	}
    }
    {
	// Cleanup exited workers.
	while (!exited_workers.empty()) {
	    exited_workers.front()->join();
	    delete exited_workers.front();
	    exited_workers.pop();
	}
    }
}

void
WorkerPool::worker_message_handled(WorkerThread * worker, bool ready_to_exit)
{
    ContextLocker lock(workerlist_mutex);

    std::map<WorkerThread *, WorkerDetails>::iterator i;
    i = workers.find(worker);
    assert(i != workers.end());
    assert(i->second.messages > 0);
    --(i->second.messages);
    if (ready_to_exit && i->second.messages == 0) {
	i->second.ready_to_exit = true;
    }
}

void
WorkerPool::worker_exited(WorkerThread * worker)
{
    ContextLocker lock(workerlist_mutex);

    if (!remove_current_worker(worker)) {
	exiting_workers.erase(worker);
    }
    // FIXME - possible memory leak here if we get an exception.
    exited_workers.push(worker);
}

void
WorkerPool::stop()
{
    ContextLocker lock(workerlist_mutex);

    logger->debug("Stopping all workers");

    while (!workers.empty()) {
	WorkerThread * worker = workers.begin()->first;
	worker->stop();

	if (!remove_current_worker(worker)) {
	    logger->error("Couldn't remove worker - not in list of current "
			  "workers.  Possible resource leak.");
	}
	try {
	    exiting_workers.insert(worker);
	} catch(...) {
	    logger->error("Couldn't add worker to list of exiting workers. "
			  "Possible resource leak.");
	    throw;
	}
    }
}

void
WorkerPool::join()
{
    ContextLocker lock(workerlist_mutex);

    logger->debug("Joining all workers");

    // Cleanup exiting workers.
    while (!exiting_workers.empty()) {
	WorkerThread * worker = *exiting_workers.begin();
	lock.unlock();
	worker->join();
	lock.lock();
	if (exiting_workers.erase(worker)) {
	    delete worker;
	    // If nothing was erased, it'll be in exited_workers now, and we'll
	    // delete it later.
	}
    }

    // Cleanup exited workers.
    //
    // No need to drop the lock while joining here, because workers shouldn't
    // try to get the lock once they're in exited_workers.
    while (!exited_workers.empty()) {
	exited_workers.front()->join();
	delete exited_workers.front();
	exited_workers.pop();
    }
}
