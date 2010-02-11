/** @file dispatch.cc
 * @brief Dispatch requests to workers.
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
#include "serverinternal.h"
#include "worker.h"
#include "workerpool.h"

class EchoWorker : public Worker {
  public:
    EchoWorker(ServerInternal * server_, WorkerPool * pool_)
	    : Worker(server_, pool_) {}
    void run();
};

class EchoWorkerFactory : public WorkerFactory {
  public:
    EchoWorkerFactory(ServerInternal * server_, WorkerPool * pool_)
	    : WorkerFactory(server_, pool_) {}
    Worker * get_worker(const std::string & group, int current_workers);
};

void
EchoWorker::run()
{
    while (true) {
	Message msg = wait_for_message(true);
	if (msg.connection_num < 0)
	    break;
	send_response(msg.connection_num, msg.msg);
    }
}

Worker *
EchoWorkerFactory::get_worker(const std::string & group, int current_workers)
{
    (void) group;
    (void) current_workers;
    return new EchoWorker(server, pool);
}

void
ServerInternal::set_worker_factory()
{
    workers.set_factory(new EchoWorkerFactory(this, &workers));
}

bool
ServerInternal::dispatch_request(int connection_num, std::string & buf)
{
    if (buf.empty()) {
	return false;
    }
    workers.send_to_worker("echo", connection_num, buf);
    buf.clear();
    return true;
}
