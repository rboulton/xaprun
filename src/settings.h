/** @file settings.h
 * @brief Settings and option parsing.
 */
/* Copyright 2009 Richard Boulton
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

#ifndef XAPSRV_INCLUDED_SETTINGS_H
#define XAPSRV_INCLUDED_SETTINGS_H

#include <string>

/** The settings used by the server.
 */
struct ServerSettings {
    /// The interface which the server listens on.
    std::string interface;

    /// The filename to write logs to.
    std::string log_filename;

    /// The port which the server listens on.
    int port;

    /// Maximum number of search workers to allow simultaneously.
    int search_workers;

    /// Maximum number of update workers to allow simultaneously.
    int update_workers;

    /// Initialise the settings to default values.
    ServerSettings();

    /** Validate the settings.
     *
     *  @retval true if the settings are valid.
     *  @retval false if the settings are invalid.
     */
    bool validate() const;

    /** Parse the command line arguments, and update the settings with them.
     *
     *  @param argc Count of the command line arguments, as passed to main()
     *  @param argv Array of command line arguments, as passed to main()
     *  @returns -1 if all arguments are ok, otherwise the status code to
     *  return with.
     */
    int parse_args(int argc, char ** argv);
};

#endif /* XAPSRV_INCLUDED_SETTINGS_H */
