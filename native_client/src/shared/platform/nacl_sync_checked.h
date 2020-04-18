/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime mutex and condition variable abstraction layer.
 * The NaClX* interfaces just invoke the no-X versions of the
 * synchronization routines, and aborts if there are any error
 * returns.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_SYNC_CHECKED_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_SYNC_CHECKED_H_

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"

#include "native_client/src/shared/platform/nacl_sync.h"

EXTERN_C_BEGIN

/*
 * These are simple checked versions of the nacl_sync API which will
 * abort on any unexpected return value.
 */

void NaClXMutexCtor(struct NaClMutex *mp);
void NaClXMutexLock(struct NaClMutex *mp);
NaClSyncStatus NaClXMutexTryLock(struct NaClMutex *mp);
void NaClXMutexUnlock(struct NaClMutex *mp);

void NaClXCondVarCtor(struct NaClCondVar *cvp);
void NaClXCondVarSignal(struct NaClCondVar *cvp);
void NaClXCondVarBroadcast(struct NaClCondVar *cvp);
void NaClXCondVarWait(struct NaClCondVar *cvp,
                      struct NaClMutex   *mp);

NaClSyncStatus NaClXCondVarTimedWaitAbsolute(
    struct NaClCondVar              *cvp,
    struct NaClMutex                *mp,
    NACL_TIMESPEC_T const           *abstime);

NaClSyncStatus NaClXCondVarTimedWaitRelative(
    struct NaClCondVar              *cvp,
    struct NaClMutex                *mp,
    NACL_TIMESPEC_T const           *reltime);

EXTERN_C_END

#ifdef __cplusplus

namespace nacl {

// RAII-based lock classes.
// TODO(sehr): replace these by chrome's AutoLock class when this doesn't cause
// all of base to be pulled into untrusted code.
class ScopedNaClMutexLock {
 public:
  explicit ScopedNaClMutexLock(struct NaClMutex* mp) : mp_(mp) {
    NaClXMutexLock(mp_);
  }

  ~ScopedNaClMutexLock() {
    NaClXMutexUnlock(mp_);
  }
 private:
  struct NaClMutex* mp_;
};

#if defined(__native_client__) || (NACL_WINDOWS == 0)
class ScopedPthreadMutexLock {
 public:
  explicit ScopedPthreadMutexLock(pthread_mutex_t* mp) : mp_(mp) {
    pthread_mutex_lock(mp_);
  }

  ~ScopedPthreadMutexLock() {
    pthread_mutex_unlock(mp_);
  }
 private:
  pthread_mutex_t* mp_;
};
#endif  // defined(__native_client__) || (NACL_WINDOWS == 0)


}  // namespace

#endif  // __cplusplus

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_SYNC_CHECKED_H_ */
