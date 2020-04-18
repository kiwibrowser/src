/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <pthread.h>
#include <errno.h>
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_sync.h"

int NaClMutexCtor(struct NaClMutex *mp) {
  if (0 != pthread_mutex_init(&mp->mu, (pthread_mutexattr_t *) 0)) {
    return 0;
  }
  return 1;
}

void NaClMutexDtor(struct NaClMutex *mp) {
  pthread_mutex_destroy(&mp->mu);
}

#define MAP(E, S) case E: do { return S; } while (0)

NaClSyncStatus NaClMutexLock(struct NaClMutex *mp) {
  switch (pthread_mutex_lock(&mp->mu)) {
    MAP(0, NACL_SYNC_OK);
    MAP(EINVAL, NACL_SYNC_MUTEX_INVALID);
    /* no EAGAIN; we don't support recursive mutexes */
    MAP(EDEADLK, NACL_SYNC_MUTEX_DEADLOCK);
  }
  return NACL_SYNC_INTERNAL_ERROR;
}

NaClSyncStatus NaClMutexTryLock(struct NaClMutex *mp) {
  switch (pthread_mutex_trylock(&mp->mu)) {
    MAP(0, NACL_SYNC_OK);
    MAP(EINVAL, NACL_SYNC_MUTEX_INVALID);
    MAP(EBUSY, NACL_SYNC_BUSY);
    /* no EAGAIN; we don't support recursive mutexes */
  }
  return NACL_SYNC_INTERNAL_ERROR;
}

NaClSyncStatus NaClMutexUnlock(struct NaClMutex *mp) {
  switch (pthread_mutex_unlock(&mp->mu)) {
    MAP(0, NACL_SYNC_OK);
    MAP(EINVAL, NACL_SYNC_MUTEX_INVALID);
    MAP(EPERM, NACL_SYNC_MUTEX_PERMISSION);
  }
  return NACL_SYNC_INTERNAL_ERROR;
}
