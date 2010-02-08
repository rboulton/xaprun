/** @file settings.cc
 * @brief Settings and option parsing.
 */
/* Copyright 2009 Richard Boulton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// Includes: config, this file's header, then local headers, then system.
#include <config.h>
#include "settings.h"

#include <cstdlib>
#include "gnu_getopt.h"
#include <iostream>
#include <xapian.h>

/// Defines needed for makemanpage
#define PROG_DESC "server for xapian"

ServerSettings::ServerSettings()
	: interface("0.0.0.0"),
	  log_filename("log"),
	  port(8080),
	  search_workers(10),
	  update_workers(1)
{
}

int
ServerSettings::parse_args(int argc, char ** argv)
{
    static const struct option longopts[] = {
	{ "help",       no_argument,            NULL, 'h' },
	{ "version",    no_argument,            NULL, 'v' },
	{ "interface",  required_argument,      NULL, 'i' },
	{ "port",       required_argument,      NULL, 'p' },
	{ "searchers",  required_argument,      NULL, 's' },
	{ "updaters",   required_argument,      NULL, 'u' },
	{ "log",        required_argument,      NULL, 'l' },
	{ 0, 0, NULL, 0 }
    };

    int getopt_ret;
    while ((getopt_ret = gnu_getopt_long(argc, argv, "hvi:p:s:u:l:", longopts, NULL)) != -1)
    {
	switch (getopt_ret) {
	    case '?': {
		// Invalid option
		return 1;
	    }
	    case 'h': {
		std::cout << "xappy-server - "PROG_DESC"\n\n"
"Usage: xappy-server [OPTIONS]\n\n"
"Options:\n"
"  -i, --interface   Set the interface to listen on\n"
"  -p, --port        Set the port to listen on\n"
"  -s, --searchers   Set the maximum number of concurrent search workers\n"
"  -u, --updaters    Set the maximum number of concurrent update workers\n"
"  -l, --log         Set the filename to write log entries to\n"
"  -h, --help        display this help and exit\n"
"  -v, --version     output version information and exit\n"
<< std::endl;
		return 0;
	    }
	    case 'v': {
		std::cout << "xappy-server version "VERSION"\n"
"Using xapian version "XAPIAN_VERSION
<< std::endl;
		return 0;
	    }
	    case 'i': {
		interface = optarg;
		break;
	    }
	    case 'p': {
		port = atoi(optarg);
		break;
	    }
	    case 's': {
		search_workers = atoi(optarg);
		break;
	    }
	    case 'u': {
		update_workers = atoi(optarg);
		break;
	    }
	    case 'l': {
		log_filename = optarg;
		break;
	    }
	}
    }
    if (!validate()) {
	return 1;
    }
    if (argc - optind > 0) {
	std::cout << "Too many arguments - use -h for help" << std::endl;
	return 1;
    }
    return -1;
}

bool
ServerSettings::validate() const
{
    bool ok = true;
    if (search_workers < 1) {
	std::cerr << "Error: must have at least one search worker - got " << search_workers << std::endl;
	ok = false;
    }
    if (update_workers < 1) {
	std::cerr << "Error: must have at least one update worker - got " << update_workers << std::endl;
	ok = false;
    }
    return ok;
}
