/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <sys/time.h>
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"

static const uint64_t kMicrosecondsPerSecond = 1000 * 1000;
static const uint64_t kNanosecondsPerMicrosecond = 1000;

/* Condition variable C API */

int NaClCondVarCtor(struct NaClCondVar  *cvp) {
  if (0 != pthread_cond_init(&cvp->cv, (pthread_condattr_t *) 0)) {
    return 0;
  }
  return 1;
}

void NaClCondVarDtor(struct NaClCondVar *cvp) {
  pthread_cond_destroy(&cvp->cv);
}

NaClSyncStatus NaClCondVarSignal(struct NaClCondVar *cvp) {
  pthread_cond_signal(&cvp->cv);
  return NACL_SYNC_OK;
}

NaClSyncStatus NaClCondVarBroadcast(struct NaClCondVar *cvp) {
  pthread_cond_broadcast(&cvp->cv);
  return NACL_SYNC_OK;
}

NaClSyncStatus NaClCondVarWait(struct NaClCondVar *cvp,
                               struct NaClMutex   *mp) {
  pthread_cond_wait(&cvp->cv, &mp->mu);
  return NACL_SYNC_OK;
}

NaClSyncStatus NaClCondVarTimedWaitRelative(
    struct NaClCondVar             *cvp,
    struct NaClMutex               *mp,
    NACL_TIMESPEC_T const          *reltime) {
  uint64_t relative_wait_us =
      reltime->tv_sec * kMicrosecondsPerSecond +
      reltime->tv_nsec / kNanosecondsPerMicrosecond;
  uint64_t current_time_us;
  uint64_t wakeup_time_us;
  int result;
  struct timespec ts;
  struct timeval tv;
  struct timezone tz = { 0, 0 };  /* UTC */
  if (gettimeofday(&tv, &tz) == 0) {
    current_time_us = tv.tv_sec * kMicrosecondsPerSecond + tv.tv_usec;
  } else {
    return NACL_SYNC_INTERNAL_ERROR;
  }
  wakeup_time_us = current_time_us + relative_wait_us;
  ts.tv_sec = wakeup_time_us / kMicrosecondsPerSecond;
  ts.tv_nsec = (wakeup_time_us - ts.tv_sec * kMicrosecondsPerSecond) *
               kNanosecondsPerMicrosecond;

  result = pthread_cond_timedwait(&cvp->cv, &mp->mu, &ts);
  if (0 == result) {
    return NACL_SYNC_OK;
  }
  return NACL_SYNC_CONDVAR_TIMEDOUT;
}

NaClSyncStatus NaClCondVarTimedWaitAbsolute(
    struct NaClCondVar              *cvp,
    struct NaClMutex                *mp,
    NACL_TIMESPEC_T const           *abstime) {
  struct timespec ts;
  int result;
  ts.tv_sec = abstime->tv_sec;
  ts.tv_nsec = abstime->tv_nsec;
  result = pthread_cond_timedwait(&cvp->cv, &mp->mu, &ts);
  if (0 == result) {
    return NACL_SYNC_OK;
  }
  return NACL_SYNC_CONDVAR_TIMEDOUT;
}
