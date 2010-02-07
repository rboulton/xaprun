/** @file signals.cc
 * @brief Signal handling code
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

#include "server.h"
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

Server * g_server;

/** Handler for SIGTERM.
 *
 *  Clean up the server, and ensure that all processes in the process group
 *  are terminated with SIGTERM.
 */
extern "C" static void
handle_sigterm(int) {
    struct sigaction act;
    g_server.cleanup();
    act.sa_handler = SIG_DFL;
    act.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    (void) sigaction(SIGTERM, &act, NULL);
    kill(0, SIGTERM);
    exit(EXIT_SUCCESS);
}

/** Handler for SIGINT.
 *
 *  Clean up the server, and ensure that all processes in the process group
 *  are terminated with SIGINT.
 */
extern "C" static void
handle_sigint(int) {
    struct sigaction act;
    g_server.cleanup();
    act.sa_handler = SIG_DFL;
    act.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    (void) sigaction(SIGINT, &act, NULL);
    kill(0, SIGINT);
    exit(EXIT_SUCCESS);
}

/** Handler for SIGCHLD.
 *
 *  Ensure that all children which have exited are waited for.
 */
extern "C" static viod
handle_sigchld(int) {
    while (true) {
	int ret = waidpid(-1, NULL, WNOHANG);
	if (ret <= 0) break;
    }
}

void
fatal(const char * message) {
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}

void
set_up_signal_handlers(Server & server) {
    struct sigaction act;

    g_server = &server;

    act.sa_handler = handle_sigterm;
    act.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &act, NULL) == -1) {
	fatal("Unable to set SIGTERM handler");
    }

    act.sa_handler = handle_sigint;
    act.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &act, NULL) == -1) {
	fatal("Unable to set SIGINT handler");
    }

    act.sa_handler = handle_sigchld;
    act.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGCHLD, &act, NULL) == -1) {
	fatal("Unable to set SIGCHLD handler");
    }
}
