/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_CLOCK_H_
#define NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_CLOCK_H_

/*
 * A platform-abstracted implementation of clock_getres,
 * clock_gettime, clock_settime, providing CLOCK_REALTIME
 * (gettimeofday equivalent) and CLOCK_MONOTONIC /
 * CLOCK_MONOTONIC_RAW.
 */

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"

EXTERN_C_BEGIN

typedef enum NaClClockIdEnum {
  NACL_CLOCK_REALTIME = NACL_ABI_CLOCK_REALTIME,
  NACL_CLOCK_MONOTONIC = NACL_ABI_CLOCK_MONOTONIC,
  NACL_CLOCK_PROCESS_CPUTIME_ID = NACL_ABI_CLOCK_PROCESS_CPUTIME_ID,
  NACL_CLOCK_THREAD_CPUTIME_ID = NACL_ABI_CLOCK_THREAD_CPUTIME_ID
} nacl_clockid_t;

/*
 * NaClClockInit should be invoked *after* NaClTimeInit.  Using
 * NaClPlatformInit will do the right thing.
 *
 * Returns bool-as-int, with true for success -- if NaClClockInit
 * fails, it's pretty catastrophic and the NaClClock module is
 * unusable.
 */
int NaClClockInit(void);

void NaClClockFini(void);

int NaClClockGetRes(nacl_clockid_t            clk_id,
                    struct nacl_abi_timespec  *res) NACL_WUR;

int NaClClockGetTime(nacl_clockid_t           clk_id,
                     struct nacl_abi_timespec *tp) NACL_WUR;

EXTERN_C_END

#endif
