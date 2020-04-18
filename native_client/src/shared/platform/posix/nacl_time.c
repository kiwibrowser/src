/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime time abstraction layer.
 * This is the host-OS-dependent implementation.
 */

#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/shared/platform/posix/nacl_time_types.h"

#define NANOS_PER_UNIT  (1000*1000*1000)

static struct NaClTimeState gNaClTimeState;

void NaClAllowLowResolutionTimeOfDay(void) {
/* Always use high resolution timer. */
}

void NaClTimeInternalInit(struct NaClTimeState *ntsp) {
  ntsp->time_resolution_ns = NACL_NANOS_PER_MICRO;
}

void NaClTimeInternalFini(struct NaClTimeState *ntsp) {
  UNREFERENCED_PARAMETER(ntsp);
}

uint64_t NaClTimerResolutionNsInternal(struct NaClTimeState *ntsp) {
  return ntsp->time_resolution_ns;
}

void NaClTimeInit(void) {
  NaClTimeInternalInit(&gNaClTimeState);
}

void NaClTimeFini(void) {
  NaClTimeInternalFini(&gNaClTimeState);
}

uint64_t NaClTimerResolutionNanoseconds(void) {
  return NaClTimerResolutionNsInternal(&gNaClTimeState);
}

static intptr_t NaClXlateSysRet(intptr_t rv) {
  return (rv != -1) ? rv : -NaClXlateErrno(errno);
}

int NaClGetTimeOfDay(struct nacl_abi_timeval *tv) {
  struct timeval  sys_tv;
  int             retval;

  retval = gettimeofday(&sys_tv, NULL);
  if (0 == retval) {
    tv->nacl_abi_tv_sec = sys_tv.tv_sec;
    tv->nacl_abi_tv_usec = sys_tv.tv_usec;
  }

  retval = NaClXlateSysRet(retval);
  return retval;
}

int NaClNanosleep(struct nacl_abi_timespec const  *req,
                  struct nacl_abi_timespec        *rem) {
  struct timespec host_req;
  struct timespec host_rem;
  struct timespec *host_remptr;
  int             retval;

  host_req.tv_sec = req->tv_sec;
  host_req.tv_nsec = req->tv_nsec;
  if (NULL == rem) {
    host_remptr = NULL;
  } else {
    host_remptr = &host_rem;
  }
  NaClLog(4,
          "nanosleep(%"NACL_PRIxPTR", %"NACL_PRIxPTR")\n",
          (uintptr_t) &host_req,
          (uintptr_t) host_remptr);
  NaClLog(4, "nanosleep(time = %"NACL_PRId64".%09"NACL_PRId64" S)\n",
          (int64_t) host_req.tv_sec, (int64_t) host_req.tv_nsec);
  if (host_req.tv_nsec > NANOS_PER_UNIT) {
    NaClLog(4, "tv_nsec too large %"NACL_PRId64"\n",
            (int64_t) host_req.tv_nsec);
  }
  retval = nanosleep(&host_req, host_remptr);
  NaClLog(4, " returned %d\n", retval);

  if (0 != retval && EINTR == errno && NULL != rem) {
    rem->tv_sec = host_rem.tv_sec;
    rem->tv_nsec = host_rem.tv_nsec;
  }
  retval = NaClXlateSysRet(retval);
  return retval;
}
