//
// Copyright © 2019 Jamie Madill
//
// This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
//
// glmark2 is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// glmark2.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
//  Jamie Madill
//
// From Stack Overflow under Create Commons license:
// https://stackoverflow.com/a/31335254

#ifndef SYS_TIME_H_FOR_WINDOWS_
#define SYS_TIME_H_FOR_WINDOWS_

#ifdef _WIN32
#include <time.h>
#include <windows.h>
#define CLOCK_MONOTONIC 0
inline int clock_gettime(int, struct timespec *spec)
{
    __int64 wintime;
    GetSystemTimeAsFileTime((FILETIME*)&wintime);
    wintime      -= 116444736000000000i64;           // 1601 to 1970
    spec->tv_sec  = wintime / 10000000i64;           // seconds
    spec->tv_nsec = wintime % 10000000i64 *100;      // nanoseconds
    return 0;
}
#else
#include <sys/time.h>
#endif

#endif  // SYS_TIME_H_FOR_WINDOWS_
