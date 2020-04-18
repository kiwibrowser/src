/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Semaphore Descriptor / Handle abstraction.
 */

#include "native_client/src/include/portability.h"

#include <stdlib.h>
#include <string.h>

#include "native_client/src/shared/imc/nacl_imc_c.h"

#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_semaphore.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_semaphore.h"

#include "native_client/src/trusted/service_runtime/internal_errno.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"


/*
 * This file contains the implementation for the NaClDescSemaphore subclass
 * of NaClDesc.
 *
 * NaClDescSemaphore is the subclass that wraps host-OS semaphore abstractions
 */

static struct NaClDescVtbl const kNaClDescSemaphoreVtbl;  /* fwd */

int NaClDescSemaphoreCtor(struct NaClDescSemaphore  *self, int value) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  basep->base.vtbl = (struct NaClRefCountVtbl const *) NULL;
  if (!NaClDescCtor(basep)) {
    return 0;
  }
  if (!NaClSemCtor(&self->sem, value)) {
    (*basep->base.vtbl->Dtor)(&basep->base);
    return 0;
  }

  basep->base.vtbl = (struct NaClRefCountVtbl const *) &kNaClDescSemaphoreVtbl;
  return 1;
}

void NaClDescSemaphoreDtor(struct NaClRefCount *vself) {
  struct NaClDescSemaphore *self = (struct NaClDescSemaphore *) vself;

  NaClLog(4, "NaClDescSemaphoreDtor(0x%08"NACL_PRIxPTR").\n",
          (uintptr_t) vself);
  NaClSemDtor(&self->sem);
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

int NaClDescSemaphoreFstat(struct NaClDesc          *vself,
                           struct nacl_abi_stat     *statbuf) {
  UNREFERENCED_PARAMETER(vself);

  memset(statbuf, 0, sizeof *statbuf);
  statbuf->nacl_abi_st_mode = NACL_ABI_S_IFSEMA;
  return 0;
}

int NaClDescSemaphorePost(struct NaClDesc         *vself) {
  struct NaClDescSemaphore *self = (struct NaClDescSemaphore *)vself;
  NaClSyncStatus status = NaClSemPost(&self->sem);

  return -NaClXlateNaClSyncStatus(status);
}

int NaClDescSemaphoreSemWait(struct NaClDesc          *vself) {
  struct NaClDescSemaphore *self = (struct NaClDescSemaphore *)vself;
  NaClSyncStatus status = NaClSemWait(&self->sem);

  return -NaClXlateNaClSyncStatus(status);
}

int NaClDescSemaphoreGetValue(struct NaClDesc         *vself) {
  UNREFERENCED_PARAMETER(vself);

  NaClLog(LOG_ERROR, "SemGetValue is not implemented yet\n");
  return -NACL_ABI_EINVAL;
  /*
   * TODO(gregoryd): sem_getvalue is not implemented on OSX.
   * Remove this syscall or implement it using semctl
   */
#if 0
  struct NaClDescSemaphore *self = (struct NaClDescSemaphore *) vself;
  NaClSyncStatus status = NaClSemGetValue(&self->sem);
  return NaClXlateNaClSyncStatus(status);
#endif
}


static struct NaClDescVtbl const kNaClDescSemaphoreVtbl = {
  {
    (void (*)(struct NaClRefCount *)) NaClDescSemaphoreDtor,
  },
  NaClDescMapNotImplemented,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescPReadNotImplemented,
  NaClDescPWriteNotImplemented,
  NaClDescSemaphoreFstat,
  NaClDescFchdirNotImplemented,
  NaClDescFchmodNotImplemented,
  NaClDescFsyncNotImplemented,
  NaClDescFdatasyncNotImplemented,
  NaClDescFtruncateNotImplemented,
  NaClDescGetdentsNotImplemented,
  NaClDescExternalizeSizeNotImplemented,
  NaClDescExternalizeNotImplemented,
  NaClDescLockNotImplemented,
  NaClDescTryLockNotImplemented,
  NaClDescUnlockNotImplemented,
  NaClDescWaitNotImplemented,
  NaClDescTimedWaitAbsNotImplemented,
  NaClDescSignalNotImplemented,
  NaClDescBroadcastNotImplemented,
  NaClDescSendMsgNotImplemented,
  NaClDescRecvMsgNotImplemented,
  NaClDescLowLevelSendMsgNotImplemented,
  NaClDescLowLevelRecvMsgNotImplemented,
  NaClDescConnectAddrNotImplemented,
  NaClDescAcceptConnNotImplemented,
  NaClDescSemaphorePost,
  NaClDescSemaphoreSemWait,
  NaClDescSemaphoreGetValue,
  NaClDescSetMetadata,
  NaClDescGetMetadata,
  NaClDescSetFlags,
  NaClDescGetFlags,
  NaClDescIsattyNotImplemented,
  NACL_DESC_SEMAPHORE,
};
