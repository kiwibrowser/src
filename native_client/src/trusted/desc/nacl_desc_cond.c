/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Condition Variable Descriptor / Handle abstraction.
 */

#include "native_client/src/include/portability.h"

#include <stdlib.h>
#include <string.h>

#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_mutex.h"
#include "native_client/src/trusted/desc/nacl_desc_cond.h"

#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/service_runtime/nacl_config.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"

/*
 * This file contains the implementation for the NaClDescCondVar subclass
 * of NaClDesc.
 *
 * NaClDescCondVar is the subclass that wraps host-OS condition
 * variable abstractions
 */

static struct NaClDescVtbl const kNaClDescCondVarVtbl;  /* fwd */

int NaClDescCondVarCtor(struct NaClDescCondVar  *self) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  basep->base.vtbl = (struct NaClRefCountVtbl const *) NULL;
  if (!NaClDescCtor(basep)) {
    return 0;
  }
  if (!NaClIntrCondVarCtor(&self->cv)) {
    (*basep->base.vtbl->Dtor)(&basep->base);
    return 0;
  }

  basep->base.vtbl = (struct NaClRefCountVtbl const *) &kNaClDescCondVarVtbl;
  return 1;
}

static void NaClDescCondVarDtor(struct NaClRefCount *vself) {
  struct NaClDescCondVar *self = (struct NaClDescCondVar *) vself;

  NaClLog(4, "NaClDescCondVarDtor(0x%08"NACL_PRIxPTR").\n",
          (uintptr_t) vself);
  NaClIntrCondVarDtor(&self->cv);

  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

static int NaClDescCondVarFstat(struct NaClDesc          *vself,
                                struct nacl_abi_stat     *statbuf) {
  UNREFERENCED_PARAMETER(vself);

  memset(statbuf, 0, sizeof *statbuf);
  statbuf->nacl_abi_st_mode = NACL_ABI_S_IFCOND | NACL_ABI_S_IRWXU;
  return 0;
}

static int NaClDescCondVarWait(struct NaClDesc         *vself,
                               struct NaClDesc         *mutex) {
  struct NaClDescCondVar  *self = (struct NaClDescCondVar *) vself;
  struct NaClDescMutex    *mutex_desc;
  NaClSyncStatus          status;

  if (((struct NaClDescVtbl const *) mutex->base.vtbl)->
      typeTag != NACL_DESC_MUTEX) {
    return -NACL_ABI_EINVAL;
  }
  mutex_desc = (struct NaClDescMutex *)mutex;
  status = NaClIntrCondVarWait(&self->cv,
                               &mutex_desc->mu,
                               NULL);
  return -NaClXlateNaClSyncStatus(status);
}

static int NaClDescCondVarTimedWaitAbs(struct NaClDesc                *vself,
                                       struct NaClDesc                *mutex,
                                       struct nacl_abi_timespec const *ts) {
  struct NaClDescCondVar  *self = (struct NaClDescCondVar *) vself;
  struct NaClDescMutex    *mutex_desc;
  NaClSyncStatus          status;

  if (((struct NaClDescVtbl const *) mutex->base.vtbl)->
      typeTag != NACL_DESC_MUTEX) {
    return -NACL_ABI_EINVAL;
  }
  mutex_desc = (struct NaClDescMutex *) mutex;

  status = NaClIntrCondVarWait(&self->cv,
                               &mutex_desc->mu,
                               ts);
  return -NaClXlateNaClSyncStatus(status);
}

static int NaClDescCondVarSignal(struct NaClDesc         *vself) {
  struct NaClDescCondVar *self = (struct NaClDescCondVar *) vself;
  NaClSyncStatus status = NaClIntrCondVarSignal(&self->cv);

  return -NaClXlateNaClSyncStatus(status);
}

static int NaClDescCondVarBroadcast(struct NaClDesc          *vself) {
  struct NaClDescCondVar *self = (struct NaClDescCondVar *) vself;
  NaClSyncStatus status = NaClIntrCondVarBroadcast(&self->cv);

  return -NaClXlateNaClSyncStatus(status);
}

static struct NaClDescVtbl const kNaClDescCondVarVtbl = {
  {
    NaClDescCondVarDtor,
  },
  NaClDescMapNotImplemented,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescPReadNotImplemented,
  NaClDescPWriteNotImplemented,
  NaClDescCondVarFstat,
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
  NaClDescCondVarWait,
  NaClDescCondVarTimedWaitAbs,
  NaClDescCondVarSignal,
  NaClDescCondVarBroadcast,
  NaClDescSendMsgNotImplemented,
  NaClDescRecvMsgNotImplemented,
  NaClDescLowLevelSendMsgNotImplemented,
  NaClDescLowLevelRecvMsgNotImplemented,
  NaClDescConnectAddrNotImplemented,
  NaClDescAcceptConnNotImplemented,
  NaClDescPostNotImplemented,
  NaClDescSemWaitNotImplemented,
  NaClDescGetValueNotImplemented,
  NaClDescSetMetadata,
  NaClDescGetMetadata,
  NaClDescSetFlags,
  NaClDescGetFlags,
  NaClDescIsattyNotImplemented,
  NACL_DESC_CONDVAR,
};
