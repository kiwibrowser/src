/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <time.h>
#include <errno.h>

#include "native_client/src/shared/platform/nacl_clock.h"

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/shared/platform/win/nacl_time_types.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"

/*
 * Windows does not implement POSIX.1-2001, so we must emulate the
 * clock_get{res,time} functions using Windows primitives.
 *
 * We assume that NaClTimeInit has been invoked.  This is true w/
 * NaClPlatformInit.
 */

#define MAGIC_OFFSET    (0xd00f05)  /* random offset so monotonic != real */

static int              g_NaClClock_is_initialized = 0;
struct NaClMutex        g_nacl_clock_mu;
struct nacl_abi_timeval g_nacl_clock_tv;

int NaClClockInit(void) {
  if (0 != NaClGetTimeOfDay(&g_nacl_clock_tv)) {
    return 0;
  }
  g_NaClClock_is_initialized = NaClMutexCtor(&g_nacl_clock_mu);
  return g_NaClClock_is_initialized;
}

void NaClClockFini(void) {
  NaClMutexDtor(&g_nacl_clock_mu);
}

int NaClClockGetRes(nacl_clockid_t            clk_id,
                    struct nacl_abi_timespec  *res) {
  int       rv = -NACL_ABI_EINVAL;
  uint64_t  t_resolution_ns;

  if (!g_NaClClock_is_initialized) {
    NaClLog(LOG_FATAL,
            "NaClClockGetRes invoked without successful NaClClockInit\n");
  }
  switch (clk_id) {
    case NACL_CLOCK_REALTIME:
    case NACL_CLOCK_MONOTONIC:
    case NACL_CLOCK_PROCESS_CPUTIME_ID:
    case NACL_CLOCK_THREAD_CPUTIME_ID:
      t_resolution_ns = NaClTimerResolutionNanoseconds();
      res->tv_sec  = (nacl_abi_time_t) (t_resolution_ns / NACL_NANOS_PER_UNIT);
      res->tv_nsec = (int32_t)         (t_resolution_ns % NACL_NANOS_PER_UNIT);
      /*
       * very surprised if res->tv_sec != 0, since that would be a
       * rather low resolution timer!
       */
      rv = 0;
      break;
  }

  return rv;
}

static INLINE uint64_t FiletimeToUint64(FILETIME ft) {
  return (((uint64_t) ft.dwHighDateTime << 32) | ft.dwLowDateTime);
}

int NaClClockGetTime(nacl_clockid_t           clk_id,
                     struct nacl_abi_timespec *tp) {
  int                     rv = -NACL_ABI_EINVAL;
  struct nacl_abi_timeval tv;
  uint64_t                t_mono_prev_us;
  uint64_t                t_mono_cur_us;
  FILETIME                t_creat;
  FILETIME                t_exit;
  FILETIME                t_kernel;
  FILETIME                t_user;
  uint64_t                tick_ns;

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
      /*
       * Get real time, compare with last monotonic time.  If later
       * than last monotonic time, set last monotonic time to real
       * time timestamp; otherwise we leave last monotonoic time
       * alone.  In either case, return last monotonic time.
       *
       * The interpretation used here is that "monotonic" means
       * monotonic non-decreasing, as opposed to monotonic increasing.
       * We don't assume that GetTimeOfDay only yields high-order bits
       * so we can replace low-order bits of the time value with a
       * counter to fake monotonicity.  We are dangerously close to
       * the resolution limit of 1ns imposed by the timespec structure
       * already -- it's only a few Moore's Law generations away where
       * we may have to return the same time stamp for repeated calls
       * to clock_gettime (if CPU frequency clock is continued to be
       * used to drive performance counters; RTDSC is moving to a
       * fixed rate [constant_tsc], fortunately).
       */
      rv = NaClGetTimeOfDay(&tv);
      if (0 == rv) {
        NaClXMutexLock(&g_nacl_clock_mu);
        t_mono_prev_us = g_nacl_clock_tv.nacl_abi_tv_sec * 1000000
            + g_nacl_clock_tv.nacl_abi_tv_usec;
        t_mono_cur_us  = tv.nacl_abi_tv_sec * 1000000
            + tv.nacl_abi_tv_usec;
        if (t_mono_cur_us > t_mono_prev_us) {
          g_nacl_clock_tv = tv;
        }
        tp->tv_sec = g_nacl_clock_tv.nacl_abi_tv_sec + MAGIC_OFFSET;
        tp->tv_nsec = g_nacl_clock_tv.nacl_abi_tv_usec * 1000;
        NaClXMutexUnlock(&g_nacl_clock_mu);
        rv = 0;
      }
      break;
    case NACL_CLOCK_PROCESS_CPUTIME_ID:
      if (GetProcessTimes(GetCurrentProcess(),
            &t_creat, &t_exit, &t_kernel, &t_user)) {
        tick_ns = (FiletimeToUint64(t_kernel) + FiletimeToUint64(t_user)) * 100;
        tp->tv_sec =  tick_ns / NACL_NANOS_PER_UNIT;
        tp->tv_nsec = tick_ns % NACL_NANOS_PER_UNIT;
        rv = 0;
      }
      break;
    case NACL_CLOCK_THREAD_CPUTIME_ID:
      if (GetThreadTimes(GetCurrentThread(),
            &t_creat, &t_exit, &t_kernel, &t_user)) {
        tick_ns = (FiletimeToUint64(t_kernel) + FiletimeToUint64(t_user)) * 100;
        tp->tv_sec =  tick_ns / NACL_NANOS_PER_UNIT;
        tp->tv_nsec = tick_ns % NACL_NANOS_PER_UNIT;
        rv = 0;
      }
      break;
  }
  return rv;
}
