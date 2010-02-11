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
#include "str.h"
#include "worker.h"

void
WorkerPool::add_worker(Worker * worker, const std::string & group)
{
    try {
	workers.insert(std::make_pair(worker, WorkerDetails(group)));
    } catch(...) {
	delete worker;
	throw;
    }

    // Add the worker to `workers_by_group`
    std::map<std::string, std::set<Worker *> >::iterator i;
    i = workers_by_group.find(group);
    if (i == workers_by_group.end()) {
	std::pair<std::map<std::string, std::set<Worker *> >::iterator,
		bool> j;
	j = workers_by_group.insert(
		std::make_pair(group, std::set<Worker *>()));
	i = j.first;
	assert(j.second);
    }
    i->second.insert(worker);
}

bool
WorkerPool::remove_current_worker(Worker * worker)
{
    std::map<Worker *, WorkerDetails>::iterator i;
    i = workers.find(worker);
    if (i == workers.end())
	return false;
    std::map<std::string, std::set<Worker *> >::iterator j;
    j = workers_by_group.find(i->second.group);
    if (j == workers_by_group.end())
	return false;

    std::set<Worker *>::iterator k = j->second.find(worker);
    if (k == j->second.end())
	return false;
    j->second.erase(k);
    workers.erase(i);
    return true;
}

void
WorkerPool::request_exit(Worker * worker)
{
    logger->info("Sending stop request to worker");
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
WorkerPool::do_send_to_worker(const std::string & group,
			      int connection_num,
			      const std::string & msg)
{
    // Look for a worker with no messages waiting.
    std::map<Worker *, WorkerDetails>::iterator k;
    Worker * worker = NULL;

    std::map<std::string, std::set<Worker *> >::iterator i;
    i = workers_by_group.find(group);
    if (i != workers_by_group.end()) {
	std::set<Worker *>::iterator j;
	for (j = i->second.begin(); j != i->second.end(); ++j) {
	    k = workers.find(*j);
	    assert(k != workers.end());
	    if (k->second.messages == 0) {
		worker = k->first;
		break;
	    }
	}
    }
    if (!worker) {
	// FIXME - queue the message if can't, or shouldn't, make the
	// worker.
	logger->info("Starting new worker");
	worker = factory->get_worker(group, 0);
	add_worker(worker, group);
	k = workers.find(worker);
	worker->start();
    }
    logger->info("sending request from connection " + str(connection_num) +
		 " to worker");
    ++(k->second.messages);
    k->second.ready_to_exit = false;
    worker->send_message(connection_num, msg);
}

WorkerPool::WorkerPool(Logger * logger_)
	: logger(logger_), factory(NULL)
{
    pthread_mutex_init(&workerlist_mutex, NULL);
}

WorkerPool::~WorkerPool()
{
    pthread_mutex_destroy(&workerlist_mutex);

    {
	// Cleanup current workers.
	std::map<Worker *, WorkerDetails>::iterator i;
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
	std::set<Worker *>::iterator i;
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

    delete factory;
}

void
WorkerPool::set_factory(WorkerFactory * factory_)
{
    factory = factory_;
}

void
WorkerPool::worker_message_handled(Worker * worker, bool ready_to_exit)
{
    pthread_mutex_lock(&workerlist_mutex);
    try {
	std::map<Worker *, WorkerDetails>::iterator i;
	i = workers.find(worker);
	assert(i != workers.end());
	assert(i->second.messages > 0);
	--(i->second.messages);
	if (ready_to_exit && i->second.messages == 0) {
	    i->second.ready_to_exit = true;
	}
    } catch(...) {
	pthread_mutex_unlock(&workerlist_mutex);
	throw;
    }
    pthread_mutex_unlock(&workerlist_mutex);
}

void
WorkerPool::worker_exited(Worker * worker)
{
    pthread_mutex_lock(&workerlist_mutex);
    try {
	if (!remove_current_worker(worker)) {
	    exiting_workers.erase(worker);
	}
	// FIXME - possible memory leak here if we get an exception.
	exited_workers.push(worker);
    } catch(...) {
	pthread_mutex_unlock(&workerlist_mutex);
	throw;
    }
    pthread_mutex_unlock(&workerlist_mutex);
}

void
WorkerPool::send_to_worker(const std::string & group,
			      int connection_num,
			      const std::string & msg)
{
    pthread_mutex_lock(&workerlist_mutex);
    try {
	do_send_to_worker(group, connection_num, msg);
    } catch(...) {
	pthread_mutex_unlock(&workerlist_mutex);
	throw;
    }
    pthread_mutex_unlock(&workerlist_mutex);
}

