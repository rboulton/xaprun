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
#include "worker.h"
#include "workerpool.h"

#include <assert.h>
#include <errno.h>
#include "serverinternal.h"

WorkerThread::WorkerThread(ServerInternal * server_, WorkerPool * pool_,
			   Worker * worker_)
	: worker(worker_),
	  server(server_),
	  pool(pool_),
	  messages(),
	  stop_requested(false),
	  started(false),
	  joined(false),
	  had_message(false)
{
    pthread_cond_init(&message_cond, NULL);
    pthread_mutex_init(&message_mutex, NULL);
}

WorkerThread::~WorkerThread()
{
    stop();
    join();
    pthread_cond_destroy(&message_cond);
    pthread_mutex_destroy(&message_mutex);
}

Message
WorkerThread::wait_for_message(bool ready_to_exit)
{
    Message result;

    if (had_message) {
	// Tell the pool we've handled a message.
	//
	// This isn't done when the mutex is held to avoid deadlock - the pool may
	// know that the ready_to_exit flag isn't correct because it's already sent
	// a message to the worker which the worker hasn't handled, but it will
	// deal with that.
	pool->worker_message_handled(this, ready_to_exit);
    } else {
	had_message = true;
    }

    if (pthread_mutex_lock(&message_mutex) != 0)
	throw StopWorkerException();
    while (!stop_requested && messages.empty()) {
	// Worker is idle
	(void)pthread_cond_wait(&message_cond, &message_mutex);
    }
    if (stop_requested) {
	(void) pthread_mutex_unlock(&message_mutex);
	throw StopWorkerException();
    }
    result = messages.front();
    messages.pop();
    if (pthread_mutex_unlock(&message_mutex) != 0)
	throw StopWorkerException();
    return result;
}

void
WorkerThread::send_response(int connection_num, const std::string & msg)
{
    server->queue_response(connection_num, msg);
}

static void *
run_worker_thread(void * arg_ptr)
{
    WorkerThread * worker = reinterpret_cast<WorkerThread*>(arg_ptr);
    worker->do_run();
    return NULL;
}

void
WorkerThread::do_run()
{
    try {
	worker->run();
    } catch (StopWorkerException & e) {
	// Do nothing
    }
    worker->cleanup();
    pool->worker_exited(this);
}

bool
WorkerThread::start()
{
    assert(!started);
    started = true;
    pool->logger->debug("Starting worker");
    int ret = pthread_create(&worker_thread, NULL, run_worker_thread, this);
    if (ret == -1) {
	server->set_sys_error("Can't create worker thread", errno);
	return false;
    }
    return true;
}

void
WorkerThread::stop()
{
    if (!started)
	return;
    if (pthread_mutex_lock(&message_mutex) != 0)
	server->set_sys_error("Can't get lock on worker to stop it", errno);
    if (!stop_requested) {
	stop_requested = true;
	pool->logger->debug("Stopping worker");
	(void) pthread_cond_signal(&message_cond);
    }
    if (pthread_mutex_unlock(&message_mutex) != 0)
	server->set_sys_error("Can't release lock on worker after stopping it",
			      errno);
}

void
WorkerThread::join()
{
    if (!started || joined)
	return;
    pool->logger->debug("Waiting for worker to stop");

    if (pthread_join(worker_thread, NULL) != 0) {
	server->set_sys_error("Failed to join worker thread", errno);
    }
    joined = true;
    pool->logger->debug("Worker stopped");
}

void
WorkerThread::send_message(const Message & msg)
{
    if (pthread_mutex_lock(&message_mutex) != 0)
	server->set_sys_error("Can't get lock on worker to send message to it",
			      errno);
    messages.push(msg);
    (void) pthread_cond_signal(&message_cond);
    if (pthread_mutex_unlock(&message_mutex) != 0)
	server->set_sys_error("Can't release lock on worker after sending "
			      "message to it", errno);
}

Message
Worker::wait_for_message(bool ready_to_exit)
{
    return thread->wait_for_message(ready_to_exit);
}

void
Worker::send_response(int connection_num, const std::string & msg)
{
    return thread->send_response(connection_num, msg);
}

void
Worker::cleanup()
{
}
