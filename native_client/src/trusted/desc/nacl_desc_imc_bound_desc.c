/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.
 * Bound IMC descriptors.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_imc.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_bound_desc.h"

#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"


static struct NaClDescVtbl const kNaClDescImcBoundDescVtbl;  /* fwd */

int NaClDescImcBoundDescCtor(struct NaClDescImcBoundDesc  *self,
                             NaClHandle                   h) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  basep->base.vtbl = (struct NaClRefCountVtbl const *) NULL;
  if (!NaClDescCtor(basep)) {
    return 0;
  }
  self->h = h;
  basep->base.vtbl = (struct NaClRefCountVtbl const *)
      &kNaClDescImcBoundDescVtbl;
  return 1;
}

static void NaClDescImcBoundDescDtor(struct NaClRefCount *vself) {
  struct NaClDescImcBoundDesc *self = (struct NaClDescImcBoundDesc *) vself;

  (void) NaClClose(self->h);
  self->h = NACL_INVALID_HANDLE;
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
  return;
}

static int NaClDescImcBoundDescFstat(struct NaClDesc          *vself,
                                     struct nacl_abi_stat     *statbuf) {
  UNREFERENCED_PARAMETER(vself);

  memset(statbuf, 0, sizeof *statbuf);
  statbuf->nacl_abi_st_mode = NACL_ABI_S_IFBOUNDSOCK;
  return 0;
}

static int NaClDescImcBoundDescAcceptConn(struct NaClDesc *vself,
                                          struct NaClDesc **result) {
  /*
   * See NaClDescConnCapConnectAddr code in nacl_desc_conn_cap.c
   */
  struct NaClDescImcBoundDesc *self;
  int                         retval;
  NaClHandle                  nh;
  int                         nbytes;
  struct NaClMessageHeader    conn_msg;
  struct NaClDescImcDesc      *peer;

  self = (struct NaClDescImcBoundDesc *) vself;

  if (NULL == (peer = malloc(sizeof *peer))) {
    return -NACL_ABI_ENOMEM;
  }

  conn_msg.iov = 0;
  conn_msg.iov_length = 0;
  conn_msg.handle_count = 1;
  conn_msg.handles = &nh;
  conn_msg.flags = 0;
  nh = NACL_INVALID_HANDLE;

  NaClLog(3,
          ("NaClDescImcBoundDescAcceptConn(0x%08"NACL_PRIxPTR"):"
           " h = %d\n"),
          (uintptr_t) vself,
          self->h);

  if (-1 == (nbytes = NaClReceiveDatagram(self->h, &conn_msg, 0))) {
    NaClLog(LOG_ERROR,
            ("NaClDescImcBoundDescAcceptConn:"
             " could not receive connection message, errno %d\n"),
            errno);
    retval = -NACL_ABI_EMFILE;
    goto cleanup;
  }
  if (0 != nbytes) {
    NaClLog(LOG_ERROR, ("NaClDescImcBoundDescAcceptConn:"
                        " connection message contains data?!?\n"));
    retval = -NACL_ABI_EMFILE;  /* TODO(bsy): better errno? */
    goto cleanup;
  }
  if (1 != conn_msg.handle_count) {
    NaClLog(LOG_ERROR, ("NaClDescImcBoundDescAcceptConn: connection"
                        " message contains %"NACL_PRIdS" descriptors?!?\n"),
            conn_msg.handle_count);
    retval = -NACL_ABI_EMFILE;  /* TODO(bsy): better errno? */
    goto cleanup;
  }
  if (!NaClDescImcDescCtor(peer, nh)) {
    retval = -NACL_ABI_EMFILE;
    goto cleanup;
  }
  nh = NACL_INVALID_HANDLE;

  *result = (struct NaClDesc *) peer;
  retval = 0;

cleanup:
  if (retval < 0) {
    if (NACL_INVALID_HANDLE != nh) {
      (void) NaClClose(nh);
      nh = NACL_INVALID_HANDLE;
    }
    free(peer);
  }
  return retval;
}

static struct NaClDescVtbl const kNaClDescImcBoundDescVtbl = {
  {
    NaClDescImcBoundDescDtor,
  },
  NaClDescMapNotImplemented,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescPReadNotImplemented,
  NaClDescPWriteNotImplemented,
  NaClDescImcBoundDescFstat,
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
  NaClDescImcBoundDescAcceptConn,
  NaClDescPostNotImplemented,
  NaClDescSemWaitNotImplemented,
  NaClDescGetValueNotImplemented,
  NaClDescSetMetadata,
  NaClDescGetMetadata,
  NaClDescSetFlags,
  NaClDescGetFlags,
  NaClDescIsattyNotImplemented,
  NACL_DESC_BOUND_SOCKET,
};
