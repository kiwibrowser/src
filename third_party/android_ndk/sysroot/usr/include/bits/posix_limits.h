/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef _BITS_POSIX_LIMITS_H_
#define _BITS_POSIX_LIMITS_H_

#include <sys/cdefs.h>

#define _POSIX_VERSION 200809L
#define _POSIX2_VERSION _POSIX_VERSION
#define _XOPEN_VERSION 700

#define __BIONIC_POSIX_FEATURE_MISSING (-1)
#define __BIONIC_POSIX_FEATURE_SINCE(level) \
    (((__ANDROID_API__) >= level) ? _POSIX_VERSION : __BIONIC_POSIX_FEATURE_MISSING)

/* Availability macros. */
/* See http://man7.org/linux/man-pages/man7/posixoptions.7.html for documentation. */
/* Keep this list sorted by name. */
#define _POSIX_ADVISORY_INFO __BIONIC_POSIX_FEATURE_SINCE(23) /* posix_memadvise arrived late. */
#define _POSIX_ASYNCHRONOUS_IO __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX_BARRIERS __BIONIC_POSIX_FEATURE_SINCE(24)
#define _POSIX_CHOWN_RESTRICTED 1 /* chown/fchown require appropriate privileges. */
#define _POSIX_CLOCK_SELECTION __BIONIC_POSIX_FEATURE_SINCE(21) /* clock_nanosleep/pthread_condattr_getclock/pthread_condattr_setclock. */
#define _POSIX_CPUTIME _POSIX_VERSION /* CLOCK_PROCESS_CPUTIME_ID. */
#define _POSIX_FSYNC _POSIX_VERSION /* fsync. */
#define _POSIX_IPV6 _POSIX_VERSION
#define _POSIX_JOB_CONTROL __BIONIC_POSIX_FEATURE_SINCE(21) /* setpgid/tcdrain/tcflush/tcgetpgrp/tcsendbreak/tcsetattr/tcsetpgrp. */
#define _POSIX_MAPPED_FILES _POSIX_VERSION /* mmap/msync/munmap. */
#define _POSIX_MEMLOCK __BIONIC_POSIX_FEATURE_SINCE(17) /* mlockall/munlockall. */
#define _POSIX_MEMLOCK_RANGE _POSIX_VERSION /* mlock/munlock. */
#define _POSIX_MEMORY_PROTECTION _POSIX_VERSION /* mprotect. */
#define _POSIX_MESSAGE_PASSING __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX_MONOTONIC_CLOCK _POSIX_VERSION /* CLOCK_MONOTONIC. */
#define _POSIX_NO_TRUNC 1 /* Over-long pathnames return errors. */
#define _POSIX_PRIORITIZED_IO __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX_PRIORITY_SCHEDULING _POSIX_VERSION /* sched_*. */
#define _POSIX_RAW_SOCKETS _POSIX_VERSION
#define _POSIX_READER_WRITER_LOCKS _POSIX_VERSION /* pthread_rwlock*. */
#define _POSIX_REALTIME_SIGNALS __BIONIC_POSIX_FEATURE_SINCE(23) /* sigqueue/sigtimedwait/sigwaitinfo. */
#define _POSIX_REGEXP 1
#define _POSIX_SAVED_IDS 1
#define _POSIX_SEMAPHORES _POSIX_VERSION /* sem_*. */
#define _POSIX_SHARED_MEMORY_OBJECTS __BIONIC_POSIX_FEATURE_MISSING /* mmap/munmap are implemented, but shm_open/shm_unlink are not. */
#define _POSIX_SHELL 1 /* system. */
#define _POSIX_SPAWN __BIONIC_POSIX_FEATURE_SINCE(28) /* <spawn.h> */
#define _POSIX_SPIN_LOCKS __BIONIC_POSIX_FEATURE_SINCE(24) /* pthread_spin_*. */
#define _POSIX_SPORADIC_SERVER __BIONIC_POSIX_FEATURE_MISSING /* No SCHED_SPORADIC. */
#define _POSIX_SYNCHRONIZED_IO _POSIX_VERSION
#define _POSIX_THREAD_ATTR_STACKADDR _POSIX_VERSION /* Strictly, we're missing the deprecated pthread_attr_getstackaddr/pthread_attr_setstackaddr, but we do have pthread_attr_getstack/pthread_attr_setstack. */
#define _POSIX_THREAD_ATTR_STACKSIZE _POSIX_VERSION /* pthread_attr_getstack/pthread_attr_getstacksize/pthread_attr_setstack/pthread_attr_setstacksize. */
#define _POSIX_THREAD_CPUTIME _POSIX_VERSION /* CLOCK_THREAD_CPUTIME_ID. */
#define _POSIX_THREAD_PRIO_INHERIT __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX_THREAD_PRIO_PROTECT __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX_THREAD_PRIORITY_SCHEDULING _POSIX_VERSION /* Strictly, pthread_attr_getinheritsched/pthread_attr_setinheritsched are missing. */
#define _POSIX_THREAD_PROCESS_SHARED _POSIX_VERSION
#define _POSIX_THREAD_ROBUST_PRIO_INHERIT __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX_THREAD_ROBUST_PRIO_PROTECT __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX_THREAD_SAFE_FUNCTIONS _POSIX_VERSION
#define _POSIX_THREAD_SPORADIC_SERVER __BIONIC_POSIX_FEATURE_MISSING /* No SCHED_SPORADIC. */
#define _POSIX_THREADS _POSIX_VERSION /* Strictly, pthread_cancel/pthread_testcancel are missing. */
#define _POSIX_TIMEOUTS __BIONIC_POSIX_FEATURE_SINCE(21) /* pthread_mutex_timedlock arrived late. */
#define _POSIX_TIMERS _POSIX_VERSION /* clock_getres/clock_gettime/clock_settime/nanosleep/timer_create/timer_delete/timer_gettime/timer_getoverrun/timer_settime. */
#define _POSIX_TRACE __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX_TRACE_EVENT_FILTER __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX_TRACE_INHERIT __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX_TRACE_LOG __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX_TYPED_MEMORY_OBJECTS __BIONIC_POSIX_FEATURE_MISSING

#define _POSIX_VDISABLE             '\0'

#define _POSIX2_C_BIND              _POSIX_VERSION
#define _POSIX2_C_DEV               __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX2_CHAR_TERM           _POSIX_VERSION
#define _POSIX2_FORT_DEV            __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX2_FORT_RUN            __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX2_LOCALEDEF           __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX2_SW_DEV              __BIONIC_POSIX_FEATURE_MISSING
#define _POSIX2_UPE                 __BIONIC_POSIX_FEATURE_MISSING

#if defined(__LP64__)
#define _POSIX_V7_ILP32_OFF32       -1
#define _POSIX_V7_ILP32_OFFBIG      -1
#define _POSIX_V7_LP64_OFF64         1
#define _POSIX_V7_LPBIG_OFFBIG       1
#else
#define _POSIX_V7_ILP32_OFF32        1
#define _POSIX_V7_ILP32_OFFBIG      -1
#define _POSIX_V7_LP64_OFF64        -1
#define _POSIX_V7_LPBIG_OFFBIG      -1
#endif

#define _XOPEN_CRYPT                __BIONIC_POSIX_FEATURE_MISSING
#define _XOPEN_ENH_I18N             1
#define _XOPEN_LEGACY               __BIONIC_POSIX_FEATURE_MISSING
#define _XOPEN_REALTIME             1
#define _XOPEN_REALTIME_THREADS     1
#define _XOPEN_SHM                  1
#define _XOPEN_STREAMS              __BIONIC_POSIX_FEATURE_MISSING
#define _XOPEN_UNIX                 1

/* Minimum values for other maxima. These numbers are simply lower bounds mandated by POSIX. */
/* The constant values here are explicitly specified by POSIX, not implementation dependent. */
#define _POSIX_AIO_LISTIO_MAX       2
#define _POSIX_AIO_MAX              1
#define _POSIX_ARG_MAX              4096
#define _POSIX_CHILD_MAX            25
#define _POSIX_CLOCKRES_MIN         20000000
#define _POSIX_DELAYTIMER_MAX       32
#define _POSIX_HOST_NAME_MAX        255
#define _POSIX_LINK_MAX             8
#define _POSIX_LOGIN_NAME_MAX       9
#define _POSIX_MAX_CANON            255
#define _POSIX_MAX_INPUT            255
#define _POSIX_MQ_OPEN_MAX          8
#define _POSIX_MQ_PRIO_MAX          32
#define _POSIX_NAME_MAX             14
#define _POSIX_NGROUPS_MAX          8
#define _POSIX_OPEN_MAX             20
#define _POSIX_PATH_MAX             256
#define _POSIX_PIPE_BUF             512
#define _POSIX_RE_DUP_MAX           255
#define _POSIX_RTSIG_MAX            8
#define _POSIX_SEM_NSEMS_MAX        256
#define _POSIX_SEM_VALUE_MAX        32767
#define _POSIX_SIGQUEUE_MAX         32
#define _POSIX_SSIZE_MAX            32767
#define _POSIX_STREAM_MAX           8
#define _POSIX_SS_REPL_MAX          4
#define _POSIX_SYMLINK_MAX          255
#define _POSIX_SYMLOOP_MAX          8
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS 4
#define _POSIX_THREAD_KEYS_MAX      128
#define _POSIX_THREAD_THREADS_MAX   64
#define _POSIX_TIMER_MAX            32
#define _POSIX_TRACE_EVENT_NAME_MAX 30
#define _POSIX_TRACE_NAME_MAX       8
#define _POSIX_TRACE_SYS_MAX        8
#define _POSIX_TRACE_USER_EVENT_MAX 32
#define _POSIX_TTY_NAME_MAX         9
#define _POSIX_TZNAME_MAX           6
#define _POSIX2_BC_BASE_MAX         99
#define _POSIX2_BC_DIM_MAX          2048
#define _POSIX2_BC_SCALE_MAX        99
#define _POSIX2_BC_STRING_MAX       1000
#define _POSIX2_CHARCLASS_NAME_MAX  14
#define _POSIX2_COLL_WEIGHTS_MAX    2
#define _POSIX2_EXPR_NEST_MAX       32
#define _POSIX2_LINE_MAX            2048
#define _POSIX2_RE_DUP_MAX          255
#define _XOPEN_IOV_MAX              16
#define _XOPEN_NAME_MAX             255
#define _XOPEN_PATH_MAX             1024

#endif
