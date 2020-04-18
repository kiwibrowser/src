/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <linux/time.h>

/* POSIX says <sys/time.h> gets you most of <sys/select.h> and may get you all of it. */
#include <sys/select.h>

__BEGIN_DECLS

int gettimeofday(struct timeval* __tv, struct timezone* __tz);
int settimeofday(const struct timeval* __tv, const struct timezone* __tz);

int getitimer(int __which, struct itimerval* __current_value);
int setitimer(int __which, const struct itimerval* __new_value, struct itimerval* __old_value);

int utimes(const char* __path, const struct timeval __times[2]);

#if defined(__USE_BSD)

#if __ANDROID_API__ >= 26
int futimes(int __fd, const struct timeval __times[2]) __INTRODUCED_IN(26);
int lutimes(const char* __path, const struct timeval __times[2]) __INTRODUCED_IN(26);
#endif /* __ANDROID_API__ >= 26 */

#endif

#if defined(__USE_GNU)

#if __ANDROID_API__ >= 26
int futimesat(int __dir_fd, const char* __path, const struct timeval __times[2]) __INTRODUCED_IN(26);
#endif /* __ANDROID_API__ >= 26 */

#endif

#define timerclear(a)   \
        ((a)->tv_sec = (a)->tv_usec = 0)

#define timerisset(a)    \
        ((a)->tv_sec != 0 || (a)->tv_usec != 0)

#define timercmp(a, b, op)               \
        ((a)->tv_sec == (b)->tv_sec      \
        ? (a)->tv_usec op (b)->tv_usec   \
        : (a)->tv_sec op (b)->tv_sec)

#define timeradd(a, b, res)                           \
    do {                                              \
        (res)->tv_sec  = (a)->tv_sec  + (b)->tv_sec;  \
        (res)->tv_usec = (a)->tv_usec + (b)->tv_usec; \
        if ((res)->tv_usec >= 1000000) {              \
            (res)->tv_usec -= 1000000;                \
            (res)->tv_sec  += 1;                      \
        }                                             \
    } while (0)

#define timersub(a, b, res)                           \
    do {                                              \
        (res)->tv_sec  = (a)->tv_sec  - (b)->tv_sec;  \
        (res)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        if ((res)->tv_usec < 0) {                     \
            (res)->tv_usec += 1000000;                \
            (res)->tv_sec  -= 1;                      \
        }                                             \
    } while (0)

#define TIMEVAL_TO_TIMESPEC(tv, ts) {     \
    (ts)->tv_sec = (tv)->tv_sec;          \
    (ts)->tv_nsec = (tv)->tv_usec * 1000; \
}
#define TIMESPEC_TO_TIMEVAL(tv, ts) {     \
    (tv)->tv_sec = (ts)->tv_sec;          \
    (tv)->tv_usec = (ts)->tv_nsec / 1000; \
}

__END_DECLS

#endif
