/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl semaphore implementation (Linux)
 */

#include <errno.h>
#include <unistd.h>

#include "native_client/src/shared/platform/linux/nacl_semaphore.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"

int NaClSemCtor(struct NaClSemaphore *sem, int32_t value) {
  /*
   * TODO(gregoryd): make sure that sem_init behaves identically on all
   * platforms. Examples: negative values, very large value.
   */
  return (0 == sem_init(&sem->sem_obj, 0, value));
}

void NaClSemDtor(struct NaClSemaphore *sem) {
  sem_destroy(&sem->sem_obj);
}

NaClSyncStatus NaClSemWait(struct NaClSemaphore *sem) {
  sem_wait(&sem->sem_obj);  /* always returns 0 */
  return NACL_SYNC_OK;
}

NaClSyncStatus NaClSemTryWait(struct NaClSemaphore *sem) {
  if (0 == sem_trywait(&sem->sem_obj)) {
    return NACL_SYNC_OK;
  }
  return NACL_SYNC_BUSY;
}

NaClSyncStatus NaClSemPost(struct NaClSemaphore *sem) {
  if (0 == sem_post(&sem->sem_obj)) {
    return NACL_SYNC_OK;
  }
  /* Posting above SEM_MAX_VALUE does not always fail, but sometimes it may */
  if ((ERANGE == errno) || (EOVERFLOW == errno)) {
    return NACL_SYNC_SEM_RANGE_ERROR;
  }
  return NACL_SYNC_INTERNAL_ERROR;
}

int32_t NaClSemGetValue(struct NaClSemaphore *sem) {
  int32_t value;
  sem_getvalue(&sem->sem_obj, &value);  /* always returns 0 */
  return value;
}
