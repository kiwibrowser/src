/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl semaphore implementation (OSX)
 */

#include <errno.h>
#include <unistd.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/shared/platform/nacl_global_secure_random.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/osx/nacl_semaphore.h"
/*
* NOTE(gregoryd): following Gears in defining SEM_NAME_LEN:
* Gears: The docs claim that SEM_NAME_LEN should be defined.  It is not.
* However, by looking at the xnu source (bsd/kern/posix_sem.c),
* it defines PSEMNAMLEN to be 31 characters.  We'll use that value.
*/
#define SEM_NAME_LEN 31


#if NACL_USE_NATIVE_SEMAPHORES

int NaClSemCtor(struct NaClSemaphore *sem, int32_t value) {
  /*
   * There are 62^30 possible names - should be enough.
   * 62 = 26*2 + 10 - the number of alphanumeric characters
   * 30 = SEM_NAME_LEN - 1
   */
  char sem_name[SEM_NAME_LEN];

  do {
    NaClGenerateRandomPath(&sem_name[0], SEM_NAME_LEN);
    sem->sem_descriptor = sem_open(sem_name, O_CREAT | O_EXCL, 700, value);
  } while ((SEM_FAILED == sem->sem_descriptor) && (EEXIST == errno));
  if (SEM_FAILED == sem->sem_descriptor) {
    NaClLog(1, "NaClSemCtor: sem_open failed, errno %d\n", errno);
    return 0;
  }
  sem_unlink(sem_name);
  return 1;
}

void NaClSemDtor(struct NaClSemaphore *sem) {
  sem_close(sem->sem_descriptor);
}

NaClSyncStatus NaClSemWait(struct NaClSemaphore *sem) {
  sem_wait(sem->sem_descriptor);  /* always returns 0 */
  return NACL_SYNC_OK;
}

NaClSyncStatus NaClSemTryWait(struct NaClSemaphore *sem) {
  if (0 == sem_trywait(sem->sem_descriptor)) {
    return NACL_SYNC_OK;
  }
  return NACL_SYNC_BUSY;
}

NaClSyncStatus NaClSemPost(struct NaClSemaphore *sem) {
  if (0 == sem_post(sem->sem_descriptor)) {
    return NACL_SYNC_OK;
  }
  /* Posting above SEM_MAX_VALUE does not always fail, but sometimes it may */
  if ((ERANGE == errno) || (EOVERFLOW == errno)) {
    return NACL_SYNC_SEM_RANGE_ERROR;
  }
  return NACL_SYNC_INTERNAL_ERROR;
}

int32_t NaClSemGetValue(struct NaClSemaphore *sem) {
  UNREFERENCED_PARAMETER(sem);
  return -1;
  /*
  * TODO(gregoryd) - sem_getvalue is declared but not implemented on OSX
  * Remove it from the API or reimplement.
  */
#if 0
  int32_t value;
  sem_getvalue(sem->sem_descriptor, &value);  /* always returns 0 */
  return value;
#endif
}

#else  /* NACL_USE_NATIVE_SEMAPHORES */

int NaClSemCtor(struct NaClSemaphore *sem, int32_t value) {
  if (value < 0) {
    return 0;
  }
  if (!NaClMutexCtor(&sem->mu)) {
    return 0;
  }
  if (!NaClCondVarCtor(&sem->cv)) {
    NaClMutexDtor(&sem->mu);
    return 0;
  }
  sem->value = value;
  return 1;
}

void NaClSemDtor(struct NaClSemaphore *sem) {
  NaClCondVarDtor(&sem->cv);
  NaClMutexDtor(&sem->mu);
}

NaClSyncStatus NaClSemWait(struct NaClSemaphore *sem) {
  NaClXMutexLock(&sem->mu);
  while (0 == sem->value) {
    NaClXCondVarWait(&sem->cv, &sem->mu);
  }
  sem->value--;
  NaClXMutexUnlock(&sem->mu);
  return NACL_SYNC_OK;
}

NaClSyncStatus NaClSemTryWait(struct NaClSemaphore *sem) {
  NaClSyncStatus rv = NACL_SYNC_INTERNAL_ERROR;

  NaClXMutexLock(&sem->mu);
  if (0 == sem->value) {
    rv = NACL_SYNC_BUSY;
  } else {
    sem->value--;
    rv = NACL_SYNC_OK;
  }
  NaClXMutexUnlock(&sem->mu);
  return rv;
}

NaClSyncStatus NaClSemPost(struct NaClSemaphore *sem) {
  NaClSyncStatus rv = NACL_SYNC_INTERNAL_ERROR;

  NaClXMutexLock(&sem->mu);
  if (NACL_MAX_VAL(int32_t) == sem->value) {
    rv = NACL_SYNC_SEM_RANGE_ERROR;
  } else {
    sem->value++;
    NaClXCondVarSignal(&sem->cv);
    rv = NACL_SYNC_OK;
  }
  NaClXMutexUnlock(&sem->mu);
  return rv;
}

int32_t NaClSemGetValue(struct NaClSemaphore *sem) {
  int32_t val;

  NaClXMutexLock(&sem->mu);
  val = sem->value;
  NaClXMutexUnlock(&sem->mu);
  return val;
}

#endif  /* NACL_USE_NATIVE_SEMAPHORES */
