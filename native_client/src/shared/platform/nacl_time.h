/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime time abstraction layer.
 * This is the host-OS-independent interface.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_TIME_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_TIME_H_

#include "native_client/src/include/nacl_base.h"

#include "native_client/src/trusted/service_runtime/include/sys/time.h"

EXTERN_C_BEGIN

void NaClTimeInit(void);

void NaClTimeFini(void);

/*
 * Allow use of low resolution timer to update time of day. This
 * must be called before NaClTimeInit().
 */
void NaClAllowLowResolutionTimeOfDay(void);

uint64_t NaClTimerResolutionNanoseconds(void);

int NaClGetTimeOfDay(struct nacl_abi_timeval *tv);

/* Convenience function */
int64_t NaClGetTimeOfDayMicroseconds(void);

int NaClNanosleep(struct nacl_abi_timespec const  *req,
                  struct nacl_abi_timespec        *rem);

/* internal / testing APIs */
struct NaClTimeState;  /* defined in platform-specific directory */

void NaClTimeInternalInit(struct NaClTimeState *);
void NaClTimeInternalFini(struct NaClTimeState *);
uint64_t NaClTimerResolutionNsInternal(struct NaClTimeState *);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_TIME_H_ */
