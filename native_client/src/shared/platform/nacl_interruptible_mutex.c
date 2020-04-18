/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime interruptible binary mutex, based on nacl_sync
 * interface.
 */

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_interruptible_mutex.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"


int NaClIntrMutexCtor(struct NaClIntrMutex *mp) {
  if (!NaClMutexCtor(&mp->mu)) {
    return 0;
  }
  if (!NaClCondVarCtor(&mp->cv)) {
    NaClMutexDtor(&mp->mu);
    return 0;
  }
  mp->lock_state = NACL_INTR_LOCK_FREE;
  return 1;
}

void NaClIntrMutexDtor(struct NaClIntrMutex *mp) {
  NaClCondVarDtor(&mp->cv);
  NaClMutexDtor(&mp->mu);
}

NaClSyncStatus NaClIntrMutexLock(struct NaClIntrMutex *mp) {
  NaClSyncStatus rv = NACL_SYNC_INTERNAL_ERROR;
  NaClXMutexLock(&mp->mu);
  while (NACL_INTR_LOCK_HELD == mp->lock_state) {
    NaClXCondVarWait(&mp->cv, &mp->mu);
  }
  if (NACL_INTR_LOCK_FREE == mp->lock_state) {
    mp->lock_state = NACL_INTR_LOCK_HELD;
    rv = NACL_SYNC_OK;
  }

  if (NACL_INTR_LOCK_INTERRUPTED == mp->lock_state) {
    rv = NACL_SYNC_MUTEX_INTERRUPTED;
  }
  NaClXMutexUnlock(&mp->mu);
  return rv;
}

NaClSyncStatus NaClIntrMutexTryLock(struct NaClIntrMutex *mp) {
  NaClSyncStatus rv = NACL_SYNC_INTERNAL_ERROR;

  NaClXMutexLock(&mp->mu);
  switch (mp->lock_state) {
    case NACL_INTR_LOCK_FREE:
      mp->lock_state = NACL_INTR_LOCK_HELD;
      rv = NACL_SYNC_OK;
      break;
    case NACL_INTR_LOCK_HELD:
      rv = NACL_SYNC_BUSY;
      break;
    case NACL_INTR_LOCK_INTERRUPTED:
      rv = NACL_SYNC_MUTEX_INTERRUPTED;
      break;
    default:
      rv = NACL_SYNC_INTERNAL_ERROR;
      break;
  }
  NaClXMutexUnlock(&mp->mu);
  return rv;
}

NaClSyncStatus NaClIntrMutexUnlock(struct NaClIntrMutex *mp) {
  NaClSyncStatus rv = NACL_SYNC_INTERNAL_ERROR;
  NaClXMutexLock(&mp->mu);
  if (NACL_INTR_LOCK_HELD != mp->lock_state) {
    NaClLog(1, "NaClIntrMutxUnlock: unlocking when lock is not held\n");
    rv = NACL_SYNC_MUTEX_PERMISSION;
  } else {
    rv = NACL_SYNC_OK;
  }
  mp->lock_state = NACL_INTR_LOCK_FREE;

  NaClXCondVarSignal(&mp->cv);
  NaClXMutexUnlock(&mp->mu);
  return rv;
}

void NaClIntrMutexIntr(struct NaClIntrMutex *mp) {
  NaClXMutexLock(&mp->mu);
  if (NACL_INTR_LOCK_HELD == mp->lock_state) {
    /* potentially there are threads waiting for this thread */
    mp->lock_state = NACL_INTR_LOCK_INTERRUPTED;
    NaClXCondVarBroadcast(&mp->cv);
  } else {
    mp->lock_state = NACL_INTR_LOCK_INTERRUPTED;
  }
  NaClXMutexUnlock(&mp->mu);
}

/*
 * Reset the interruptible mutex, presumably after the condition
 * causing the interrupt has been cleared.  In our case, this would be
 * an E_MOVE_ADDRESS_SPACE induced address space move.
 *
 * This is safe to invoke only after all threads are known to be in a
 * quiescent state -- i.e., will no longer call
 * NaClIntrMutex{Try,}Lock on the interruptible mutex -- since there
 * is no guarntee that all the threads awaken by NaClIntrMutexIntr
 * have actually been run yet.
 */
void NaClIntrMutexReset(struct NaClIntrMutex *mp) {
  NaClXMutexLock(&mp->mu);
  if (NACL_INTR_LOCK_INTERRUPTED != mp->lock_state) {
    NaClLog(LOG_FATAL,
            "NaClIntrMutexReset: lock at 0x%08"NACL_PRIxPTR" not interrupted\n",
            (uintptr_t) mp);
  }
  mp->lock_state = NACL_INTR_LOCK_FREE;
  NaClXMutexUnlock(&mp->mu);
}
