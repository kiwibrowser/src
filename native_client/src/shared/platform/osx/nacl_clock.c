/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_clock.h"

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"

/*
 * OSX does not include POSIX.1-2011 functions, so we emulate using
 * Mach calls.
 */
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include <mach/thread_info.h>
#include <pthread.h>

static int                        g_NaClClock_is_initialized = 0;
static mach_timebase_info_data_t  g_NaCl_time_base_info;

int NaClClockInit(void) {
  g_NaClClock_is_initialized = (mach_timebase_info(&g_NaCl_time_base_info)
                                == KERN_SUCCESS);

  return g_NaClClock_is_initialized;
}

void NaClClockFini(void) {}

int NaClClockGetRes(nacl_clockid_t            clk_id,
                    struct nacl_abi_timespec  *res) {
  int             rv = -NACL_ABI_EINVAL;
  uint64_t        t_resolution_ns;
  struct timespec host_res;

  if (!g_NaClClock_is_initialized) {
    NaClLog(LOG_FATAL,
            "NaClClockGetRes invoked without successful NaClClockInit\n");
  }
  switch (clk_id) {
    case NACL_CLOCK_REALTIME:
      t_resolution_ns = NaClTimerResolutionNanoseconds();
      host_res.tv_sec = (time_t) (t_resolution_ns / NACL_NANOS_PER_UNIT);
      host_res.tv_nsec = (long)  (t_resolution_ns % NACL_NANOS_PER_UNIT);
      rv = 0;
      break;
    case NACL_CLOCK_MONOTONIC:
      host_res.tv_sec = 0;
      /* round up */
      host_res.tv_nsec = ((g_NaCl_time_base_info.numer
                           + g_NaCl_time_base_info.denom - 1)
                          / g_NaCl_time_base_info.denom);
      rv = 0;
      break;
    case NACL_CLOCK_PROCESS_CPUTIME_ID:
    case NACL_CLOCK_THREAD_CPUTIME_ID:
      host_res.tv_sec = 0;
      host_res.tv_nsec = 1;
      rv = 0;
      break;
  }
  if (0 == rv) {
    res->tv_sec = host_res.tv_sec;
    res->tv_nsec = host_res.tv_nsec;
  }
  return rv;
}

int NaClClockGetTime(nacl_clockid_t            clk_id,
                     struct nacl_abi_timespec  *tp) {
  int                               rv = -NACL_ABI_EINVAL;
  struct nacl_abi_timeval           tv;
  uint64_t                          tick_cur;
  uint64_t                          tick_ns;
  struct task_absolutetime_info     absolutetime_info;
  thread_basic_info_data_t          _basic_info;
  thread_basic_info_t               basic_info = &_basic_info;
  mach_msg_type_number_t            count;

  if (!g_NaClClock_is_initialized) {
    NaClLog(LOG_FATAL,
            "NaClClockGetTime invoked without successful NaClClockInit\n");
  }
  switch (clk_id) {
    case NACL_CLOCK_REALTIME:
      rv = NaClGetTimeOfDay(&tv);
      if (0 == rv) {
        tp->tv_sec = tv.nacl_abi_tv_sec;
        tp->tv_nsec = tv.nacl_abi_tv_usec * 1000;
      }
      break;
    case NACL_CLOCK_MONOTONIC:
      tick_cur = mach_absolute_time();
      /*
       * mach_absolute_time() returns ticks since boot, with enough
       * bits for several hundred years if the ticks occur at one per
       * nanosecond.  numer/denom gives ns/tick, and the scaling
       * arithmetic should not result in over/underflows.
       */
      tick_ns = (tick_cur * g_NaCl_time_base_info.numer
                 / g_NaCl_time_base_info.denom);
      tp->tv_sec =  tick_ns / 1000000000;
      tp->tv_nsec = tick_ns % 1000000000;
      rv = 0;
      break;
    case NACL_CLOCK_PROCESS_CPUTIME_ID:
      count = TASK_ABSOLUTETIME_INFO_COUNT;

      if (KERN_SUCCESS != task_info(mach_task_self(),
                                    TASK_ABSOLUTETIME_INFO,
                                    (task_info_t) &absolutetime_info,
                                    &count)) {
        break;
      }
      tp->tv_sec = ((absolutetime_info.total_user
                     + absolutetime_info.total_system)
                    / NACL_NANOS_PER_UNIT);
      tp->tv_nsec = ((absolutetime_info.total_user
                      + absolutetime_info.total_system)
                     % NACL_NANOS_PER_UNIT);
      rv = 0;
      break;
    case NACL_CLOCK_THREAD_CPUTIME_ID:
      count = THREAD_BASIC_INFO_COUNT;

      /*
       * Don't use mach_thread_self() because it requires a separate
       * mach_port_deallocate() system call to release it. Instead, rely on
       * pthread's cached copy of the port.
       */
      if (KERN_SUCCESS == thread_info(pthread_mach_thread_np(pthread_self()),
                                      THREAD_BASIC_INFO,
                                      (thread_info_t) basic_info,
                                      &count)) {
        tick_ns = ((basic_info->user_time.microseconds
                    + basic_info->system_time.microseconds)
                   * NACL_NANOS_PER_MICRO);
        tp->tv_sec = (basic_info->user_time.seconds
                      + basic_info->system_time.seconds
                      + (tick_ns / NACL_NANOS_PER_UNIT));
        tp->tv_nsec = tick_ns % NACL_NANOS_PER_UNIT;
        rv = 0;
      }
      break;
  }
  return rv;
}
