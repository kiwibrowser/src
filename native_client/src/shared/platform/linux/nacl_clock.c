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
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"

/*
 * Linux is POSIX.1-2001 compliant, so the clock functions are trivial
 * mappings -- essentially we just translate NaCl ABI enums and
 * errnos.
 */
static int g_NaClClock_is_initialized = 0;

int NaClClockInit(void) { g_NaClClock_is_initialized = 1; return 1; }
void NaClClockFini(void) {}

int NaClClockGetRes(nacl_clockid_t            clk_id,
                    struct nacl_abi_timespec  *res) {
  int             rv = -NACL_ABI_EINVAL;
  struct timespec host_res;
  clockid_t       host_clk_id;

  if (!g_NaClClock_is_initialized) {
    NaClLog(LOG_FATAL,
            "NaClClockGetRes invoked without successful NaClClockInit\n");
  }
  switch (clk_id) {
    case NACL_CLOCK_REALTIME:
      host_clk_id = CLOCK_REALTIME;
      rv = 0;
      break;
    case NACL_CLOCK_MONOTONIC:
      host_clk_id = CLOCK_MONOTONIC;
      rv = 0;
      break;
    case NACL_CLOCK_PROCESS_CPUTIME_ID:
      host_clk_id = CLOCK_PROCESS_CPUTIME_ID;
      rv = 0;
      break;
    case NACL_CLOCK_THREAD_CPUTIME_ID:
      host_clk_id = CLOCK_THREAD_CPUTIME_ID;
      rv = 0;
      break;
  }
  if (0 == rv) {
    if (0 != clock_getres(host_clk_id, &host_res)) {
      rv = -NaClXlateErrno(errno);
    }
  }
  if (0 == rv) {
    res->tv_sec = host_res.tv_sec;
    res->tv_nsec = host_res.tv_nsec;
  }
  return rv;
}

int NaClClockGetTime(nacl_clockid_t            clk_id,
                     struct nacl_abi_timespec  *tp) {
  int             rv = -NACL_ABI_EINVAL;
  struct timespec host_time;

  if (!g_NaClClock_is_initialized) {
    NaClLog(LOG_FATAL,
            "NaClClockGetTime invoked without successful NaClClockInit\n");
  }
  switch (clk_id) {
    case NACL_CLOCK_REALTIME:
      if (0 != clock_gettime(CLOCK_REALTIME, &host_time)) {
        rv = -NaClXlateErrno(errno);
      } else {
        rv = 0;
      }
      break;
    case NACL_CLOCK_MONOTONIC:
      if (0 != clock_gettime(CLOCK_MONOTONIC, &host_time)) {
        rv = -NaClXlateErrno(errno);
      } else {
        rv = 0;
      }
      break;
    case NACL_CLOCK_PROCESS_CPUTIME_ID:
      /*
       * This will include the time spent in an TCB-private service thread
       * as well as the actual user threads. This not a major issue given
       * all the trade-offs of implementing the proper semantics, but it is
       * worth noting that the return value might be somewhat different from
       * what this would be in a real POSIX OS.
       */
      if (0 != clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &host_time)) {
        rv = -NaClXlateErrno(errno);
      } else {
        rv = 0;
      }
      break;
    case NACL_CLOCK_THREAD_CPUTIME_ID:
      if (0 != clock_gettime(CLOCK_THREAD_CPUTIME_ID, &host_time)) {
        rv = -NaClXlateErrno(errno);
      } else {
        rv = 0;
      }
      break;
  }
  if (0 == rv) {
    tp->tv_sec = host_time.tv_sec;
    tp->tv_nsec = host_time.tv_nsec;
  }
  return rv;
}
