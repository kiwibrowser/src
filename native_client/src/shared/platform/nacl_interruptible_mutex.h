/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime interruptible binary mutex, based on nacl_sync
 * interface.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_INTERRUPTIBLE_MUTEX_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_INTERRUPTIBLE_MUTEX_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/shared/platform/nacl_sync.h"

EXTERN_C_BEGIN

enum NaClIntrMutexState {
  NACL_INTR_LOCK_FREE,
  NACL_INTR_LOCK_HELD,
  NACL_INTR_LOCK_INTERRUPTED
};

struct NaClIntrMutex {
  /* public */
  enum NaClIntrMutexState lock_state;

  /* private */
  struct NaClMutex    mu;
  struct NaClCondVar  cv;
};

int NaClIntrMutexCtor(struct NaClIntrMutex  *mp);
/* bool success/fail */

void NaClIntrMutexDtor(struct NaClIntrMutex *mp);

NaClSyncStatus NaClIntrMutexLock(struct NaClIntrMutex  *mp);

NaClSyncStatus NaClIntrMutexTryLock(struct NaClIntrMutex *mp);

NaClSyncStatus NaClIntrMutexUnlock(struct NaClIntrMutex *mp);

void NaClIntrMutexIntr(struct NaClIntrMutex  *mp);

void NaClIntrMutexReset(struct NaClIntrMutex *mp);

EXTERN_C_END


#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_INTERRUPTIBLE_MUTEX_H_ */
