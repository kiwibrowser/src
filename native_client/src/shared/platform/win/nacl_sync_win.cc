/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * C bindings for C++ implementation of synchronization objects
 * (based on Chrome code)
 */

/* Beware: there is a delicate requirement here that the untrusted code be able
 * to include nacl_abi_* type definitions.  These headers share an include
 * guard with the exported versions, which get included first.  Unfortunately,
 * tying them to different include guards causes multiple definitions of
 * macros.
 */
#include "native_client/src/include/portability.h"
#include <sys/types.h>
#include <sys/timeb.h>

#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/shared/platform/nacl_sync.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/shared/platform/win/condition_variable.h"
#include "native_client/src/shared/platform/win/lock.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"

static const int64_t kMillisecondsPerSecond = 1000;
static const int64_t kNanosecondsPerMicrosecond = 1000;
static const int64_t kMicrosecondsPerMillisecond = 1000;
static const int64_t kMicrosecondsPerSecond =
    kMicrosecondsPerMillisecond * kMillisecondsPerSecond;

/* Mutex */
int NaClMutexCtor(struct NaClMutex *mp) {
  mp->lock = new NaCl::Lock();
  mp->held = 0;
  return NULL != mp->lock;
}

void NaClMutexDtor(struct NaClMutex *mp) {
  delete reinterpret_cast<NaCl::Lock *>(mp->lock);
  mp->lock = NULL;
}

NaClSyncStatus NaClMutexLock(struct NaClMutex *mp) {
  reinterpret_cast<NaCl::Lock *>(mp->lock)->Acquire();
  if (!mp->held) {
    mp->held = 1;
    return NACL_SYNC_OK;
  } else {
    /*
     * The only way we can successfully Acquire but have mp->held
     * already be true is because this is a recursive lock
     * acquisition.  With binary mutexes, this is a trivial deadlock.
     * Simulate the deadlock.
     */
    struct nacl_abi_timespec long_time;

    long_time.tv_sec = 10000;
    long_time.tv_nsec = 0;
    /*
     * Note that NaClLog uses mutexes internally, so this log message
     * assumes that this error is not being triggered from there.  The
     * service runtime should not deadlock in this way -- only
     * untrusted user code might.  In any case, this is "safe" in that
     * even if the NaClLog module were to have this bug, it would
     * result in an infinite log spew -- something that ought to be
     * quickly detected in normal testing.
     */
    NaClLog(LOG_WARNING,
            ("NaClMutexLock: recursive lock of binary mutex."
             "  Deadlock being simulated\n"));
    for (;;) {
      NaClNanosleep(&long_time, NULL);
    }
  }
}

NaClSyncStatus NaClMutexTryLock(struct NaClMutex *mp) {
  NaClSyncStatus status = reinterpret_cast<NaCl::Lock *>(mp->lock)->Try() ?
      NACL_SYNC_OK : NACL_SYNC_BUSY;
  if (status == NACL_SYNC_OK) {
    if (mp->held) {
      /* decrement internal recursion count in the underlying lock */
      reinterpret_cast<NaCl::Lock *>(mp->lock)->Release();
      status = NACL_SYNC_BUSY;
    }
    mp->held = 1;
  }
  return status;
}

NaClSyncStatus NaClMutexUnlock(struct NaClMutex *mp) {
  mp->held = 0;
  reinterpret_cast<NaCl::Lock *>(mp->lock)->Release();
  return NACL_SYNC_OK;
}

/* Condition variable */

int NaClCondVarCtor(struct NaClCondVar  *cvp) {
  cvp->cv = new NaCl::ConditionVariable();
  return 1;
}

void NaClCondVarDtor(struct NaClCondVar *cvp) {
  delete reinterpret_cast<NaCl::ConditionVariable*>(cvp->cv);
}

NaClSyncStatus NaClCondVarSignal(struct NaClCondVar *cvp) {
  reinterpret_cast<NaCl::ConditionVariable*>(cvp->cv)->Signal();
  return NACL_SYNC_OK;
}

NaClSyncStatus NaClCondVarBroadcast(struct NaClCondVar *cvp) {
  reinterpret_cast<NaCl::ConditionVariable*>(cvp->cv)->Broadcast();
  return NACL_SYNC_OK;
}

NaClSyncStatus NaClCondVarWait(struct NaClCondVar *cvp,
                               struct NaClMutex   *mp) {
  mp->held = 0;
  reinterpret_cast<NaCl::ConditionVariable*>(cvp->cv)->Wait(
      *(reinterpret_cast<NaCl::Lock *>(mp->lock)));
  mp->held = 1;
  return NACL_SYNC_OK;
}

NaClSyncStatus NaClCondVarTimedWaitRelative(
    struct NaClCondVar             *cvp,
    struct NaClMutex               *mp,
    struct nacl_abi_timespec const *reltime) {
  int64_t relative_usec = reltime->tv_sec * kMicrosecondsPerSecond +
                          reltime->tv_nsec / kNanosecondsPerMicrosecond;
  int result;
  mp->held = 0;
  result = (reinterpret_cast<NaCl::ConditionVariable*>(cvp->cv)->TimedWaitRel(
                *(reinterpret_cast<NaCl::Lock *>(mp->lock)),
      relative_usec));
  mp->held = 1;
  if (0 == result) {
    return NACL_SYNC_CONDVAR_TIMEDOUT;
  }
  return NACL_SYNC_OK;
}

NaClSyncStatus NaClCondVarTimedWaitAbsolute(
    struct NaClCondVar              *cvp,
    struct NaClMutex                *mp,
    struct nacl_abi_timespec const  *abstime) {
  int64_t usec = abstime->tv_sec * kMicrosecondsPerSecond +
                 abstime->tv_nsec / kNanosecondsPerMicrosecond;
  int result;
  mp->held = 0;
  result = reinterpret_cast<NaCl::ConditionVariable*>(cvp->cv)->TimedWaitAbs(
      *(reinterpret_cast<NaCl::Lock *>(mp->lock)),
      usec);
  mp->held = 1;
  if (0 == result) {
    return NACL_SYNC_CONDVAR_TIMEDOUT;
  }
  return NACL_SYNC_OK;
}
