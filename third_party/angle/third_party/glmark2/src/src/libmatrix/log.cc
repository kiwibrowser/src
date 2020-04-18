//
// Copyright (c) 2010-2012 Linaro Limited
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License which accompanies
// this distribution, and is available at
// http://www.opensource.org/licenses/mit-license.php
//
// Contributors:
//     Alexandros Frantzis <alexandros.frantzis@linaro.org>
//     Jesse Barker <jesse.barker@linaro.org>
//
#include <cstdio>
#include <cstdarg>
#include <string>
#include <sstream>
#include <iostream>
#include "log.h"

#ifdef ANDROID
#include <android/log.h>
#endif

#ifdef _WIN32
// On windows 'isatty' is found in <io.h>
#include <io.h>
#else
#include <unistd.h>
#endif

using std::string;

const string Log::continuation_prefix("\x10");
string Log::appname_;
bool Log::do_debug_(false);
std::ostream* Log::extra_out_(0);

static const string terminal_color_normal("\033[0m");
static const string terminal_color_red("\033[1;31m");
static const string terminal_color_cyan("\033[36m");
static const string terminal_color_yellow("\033[33m");
static const string empty;

static void
print_prefixed_message(std::ostream& stream, const string& color, const string& prefix,
                       const string& fmt, va_list ap)
{
    va_list aq;

    /* Estimate message size */
    va_copy(aq, ap);
    int msg_size = vsnprintf(NULL, 0, fmt.c_str(), aq);
    va_end(aq);

    /* Create the buffer to hold the message */
    char *buf = new char[msg_size + 1];

    /* Store the message in the buffer */
    va_copy(aq, ap);
    vsnprintf(buf, msg_size + 1, fmt.c_str(), aq);
    va_end(aq);

    /*
     * Print the message lines prefixed with the supplied prefix.
     * If the target stream is a terminal make the prefix colored.
     */
    string linePrefix;
    if (!prefix.empty())
    {
        static const string colon(": ");
        string start_color;
        string end_color;
        if (!color.empty())
        {
            start_color = color;
            end_color = terminal_color_normal;
        }
        linePrefix = start_color + prefix + end_color + colon;
    }

    std::string line;
    std::stringstream ss(buf);

    while(std::getline(ss, line)) {
        /*
         * If this line is a continuation of a previous log message
         * just print the line plainly.
         */
        if (line[0] == Log::continuation_prefix[0]) {
            stream << line.c_str() + 1;
        }
        else {
            /* Normal line, emit the prefix. */
            stream << linePrefix << line;
        }

        /* Only emit a newline if the original message has it. */
        if (!(ss.rdstate() & std::stringstream::eofbit))
            stream << std::endl;
    }

    delete[] buf;
}


void
Log::info(const char *fmt, ...)
{
    static const string infoprefix("Info");
    const string& prefix(do_debug_ ? infoprefix : empty);
    va_list ap;
    va_start(ap, fmt);

#ifndef ANDROID
    static const string& infocolor(isatty(fileno(stdout)) ? terminal_color_cyan : empty);
    const string& color(do_debug_ ? infocolor : empty);
    print_prefixed_message(std::cout, color, prefix, fmt, ap);
#else
    __android_log_vprint(ANDROID_LOG_INFO, appname_.c_str(), fmt, ap);
#endif

    if (extra_out_)
        print_prefixed_message(*extra_out_, empty, prefix, fmt, ap);

    va_end(ap);
}

void
Log::debug(const char *fmt, ...)
{
    static const string dbgprefix("Debug");
    if (!do_debug_)
        return;
    va_list ap;
    va_start(ap, fmt);

#ifndef ANDROID
    static const string& dbgcolor(isatty(fileno(stdout)) ? terminal_color_yellow : empty);
    print_prefixed_message(std::cout, dbgcolor, dbgprefix, fmt, ap);
#else
    __android_log_vprint(ANDROID_LOG_DEBUG, appname_.c_str(), fmt, ap);
#endif

    if (extra_out_)
        print_prefixed_message(*extra_out_, empty, dbgprefix, fmt, ap);

    va_end(ap);
}

void
Log::error(const char *fmt, ...)
{
    static const string errprefix("Error");
    va_list ap;
    va_start(ap, fmt);

#ifndef ANDROID
    static const string& errcolor(isatty(fileno(stderr)) ? terminal_color_red : empty);
    print_prefixed_message(std::cerr, errcolor, errprefix, fmt, ap);
#else
    __android_log_vprint(ANDROID_LOG_ERROR, appname_.c_str(), fmt, ap);
#endif

    if (extra_out_)
        print_prefixed_message(*extra_out_, empty, errprefix, fmt, ap);

    va_end(ap);
}

void
Log::flush()
{
#ifndef ANDROID
    std::cout.flush();
    std::cerr.flush();
#endif
    if (extra_out_)
        extra_out_->flush();
}
