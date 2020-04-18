/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_PLATFORM_WIN_NACL_TIME_TYPES_H_
#define NATIVE_CLIENT_SRC_SHARED_PLATFORM_WIN_NACL_TIME_TYPES_H_

#include <windows.h>
#include <sys/timeb.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_base.h"

/* get NaClMutex declaration, since we need size */
#include "native_client/src/shared/platform/nacl_sync.h"

EXTERN_C_BEGIN

/*
 * NaClTimeState definition for Windows
 */

struct NaClTimeState {
  /*
   * The following are used to set/reset the multimedia timer
   * resolution to the highest possible.  Only used/modified during
   * module initialization/finalization, so can be accessed w/o locks
   * during normal operation.
   */
  UINT      wPeriodMin;
  uint64_t  time_resolution_ns;
  uint32_t  allow_low_resolution;

  /*
   * The following are used to provide millisecond resolution
   * gettimeofday, and are protected by the mutex lock.
   */
  struct NaClMutex  mu;
  uint64_t          system_time_start_ms;
  DWORD             ms_counter_start;
  uint64_t          epoch_start_ms;

  /*
   * Members in this block should have valid values iff !allow_low_resolution,
   * and are undefined o/w, with exception of can_use_qpc
   * (can_use_qpc==0 if allow_low_resolution). Actually qpc_frequency,
   * qpc_start, and last_qpc are valid iff !allow_low_resolution && can_use_qpc,
   * so only can_use_qpc is well defined when !allow_low_resolution.
   */
  uint32_t          can_use_qpc;
  int64_t           qpc_frequency;
  int64_t           qpc_start;
  int64_t           last_qpc;
};

EXTERN_C_END

#endif
