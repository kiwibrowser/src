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

#ifndef _TIME_H_
#define _TIME_H_

#include <sys/cdefs.h>
#include <sys/time.h>
#include <xlocale.h>

__BEGIN_DECLS

#define CLOCKS_PER_SEC 1000000

extern char* tzname[];
extern int daylight;
extern long int timezone;

struct sigevent;

struct tm {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
  long int tm_gmtoff;
  const char* tm_zone;
};

#define TM_ZONE tm_zone

time_t time(time_t* __t);
int nanosleep(const struct timespec* __request, struct timespec* __remainder);

char* asctime(const struct tm* __tm);
char* asctime_r(const struct tm* __tm, char* __buf);

double difftime(time_t __lhs, time_t __rhs);
time_t mktime(struct tm* __tm);

struct tm* localtime(const time_t* __t);
struct tm* localtime_r(const time_t* __t, struct tm* __tm);

struct tm* gmtime(const time_t* __t);
struct tm* gmtime_r(const time_t* __t, struct tm* __tm);

char* strptime(const char* __s, const char* __fmt, struct tm* __tm);
size_t strftime(char* __buf, size_t __n, const char* __fmt, const struct tm* __tm);

#if __ANDROID_API__ >= __ANDROID_API_L__
size_t strftime_l(char* __buf, size_t __n, const char* __fmt, const struct tm* __tm, locale_t __l) __INTRODUCED_IN(21);
#else
// Implemented as static inline before 21.
#endif

char* ctime(const time_t* __t);
char* ctime_r(const time_t* __t, char* __buf);

void tzset(void);

clock_t clock(void);


#if __ANDROID_API__ >= 23
int clock_getcpuclockid(pid_t __pid, clockid_t* __clock) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */


int clock_getres(clockid_t __clock, struct timespec* __resolution);
int clock_gettime(clockid_t __clock, struct timespec* __ts);
int clock_nanosleep(clockid_t __clock, int __flags, const struct timespec* __request, struct timespec* __remainder);
int clock_settime(clockid_t __clock, const struct timespec* __ts);

int timer_create(clockid_t __clock, struct sigevent* __event, timer_t* __timer_ptr);
int timer_delete(timer_t __timer);
int timer_settime(timer_t __timer, int __flags, const struct itimerspec* __new_value, struct itimerspec* __old_value);
int timer_gettime(timer_t __timer, struct itimerspec* __ts);
int timer_getoverrun(timer_t __timer);

/* Non-standard extensions that are in the BSDs and glibc. */

#if __ANDROID_API__ >= 12
time_t timelocal(struct tm* __tm) __INTRODUCED_IN(12);
time_t timegm(struct tm* __tm) __INTRODUCED_IN(12);
#endif /* __ANDROID_API__ >= 12 */


__END_DECLS

#endif
