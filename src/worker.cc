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

#include <assert.h>
#include <errno.h>
#include "serverinternal.h"

Worker::Worker(Server::Internal * server_)
	: server(server_),
	  messages(),
	  stop_requested(false),
	  started(false)
{
    pthread_cond_init(&message_cond, NULL);
    pthread_mutex_init(&message_mutex, NULL);
}

Worker::~Worker()
{
    pthread_mutex_destroy(&message_mutex);
    pthread_cond_destroy(&message_cond);
}

std::string
Worker::wait_for_message()
{
    std::string result;
    if (pthread_mutex_lock(&message_mutex) != 0)
	throw StopWorkerException();
    while (!stop_requested && messages.empty()) {
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

static void *
run_worker_thread(void * arg_ptr)
{
    Worker * worker = reinterpret_cast<Worker*>(arg_ptr);
    try {
	worker->run();
    } catch (StopWorkerException & e) {
	// Do nothing
    }
    worker->cleanup();
    return NULL;
}

bool
Worker::start()
{
    assert(!started);
    started = true;
    int ret = pthread_create(&worker_thread, NULL, run_worker_thread, this);
    if (ret == -1) {
	server->set_sys_error("Can't create worker thread", errno);
	return false;
    }
    return true;
}

void
Worker::stop()
{
    if (!started)
	return;
    if (pthread_mutex_lock(&message_mutex) != 0)
	server->set_sys_error("Can't get lock on worker to stop it", errno);
    stop_requested = true;
    (void) pthread_cond_signal(&message_cond);
    if (pthread_mutex_unlock(&message_mutex) != 0)
	server->set_sys_error("Can't release lock on worker after stopping it",
			      errno);
}

void
Worker::join()
{
    if (!started)
	return;
    if (pthread_join(worker_thread, NULL) != 0) {
	server->set_sys_error("Failed to join worker thread", errno);
    }
}

void
Worker::send_message(const std::string & msg)
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

void
Worker::cleanup()
{
}
