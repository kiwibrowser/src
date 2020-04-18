/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Directory descriptor abstraction.
 */

#include "native_client/src/include/portability.h"

#include <stdlib.h>
#include <string.h>

#include "native_client/src/shared/imc/nacl_imc_c.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_dir.h"

#include "native_client/src/shared/platform/nacl_host_dir.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/service_runtime/internal_errno.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/dirent.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"


/*
 * This file contains the implementation for the NaClDirDesc subclass
 * of NaClDesc.
 *
 * NaClDescDirDesc is the subclass that wraps host-OS directory information.
 */

static struct NaClDescVtbl const kNaClDescDirDescVtbl;  /* fwd */

/*
 * Takes ownership of hd, will close in Dtor.
 */
int NaClDescDirDescCtor(struct NaClDescDirDesc  *self,
                        struct NaClHostDir      *hd) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  basep->base.vtbl = (struct NaClRefCountVtbl const *) NULL;
  if (!NaClDescCtor(basep)) {
    return 0;
  }
  self->hd = hd;
  basep->base.vtbl = (struct NaClRefCountVtbl const *) &kNaClDescDirDescVtbl;
  return 1;
}

static void NaClDescDirDescDtor(struct NaClRefCount *vself) {
  struct NaClDescDirDesc *self = (struct NaClDescDirDesc *) vself;

  NaClHostDirClose(self->hd);
  free(self->hd);
  self->hd = NULL;
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

struct NaClDescDirDesc *NaClDescDirDescMake(struct NaClHostDir *nhdp) {
  struct NaClDescDirDesc *ndp;

  ndp = malloc(sizeof *ndp);
  if (NULL == ndp) {
    NaClLog(LOG_FATAL,
            "NaClDescDirDescMake: no memory for 0x%08"NACL_PRIxPTR"\n",
            (uintptr_t) nhdp);
  }
  if (!NaClDescDirDescCtor(ndp, nhdp)) {
    NaClLog(LOG_FATAL,
            ("NaClDescDirDescMake:"
             " NaClDescDirDescCtor(0x%08"NACL_PRIxPTR",0x%08"NACL_PRIxPTR
             ") failed\n"),
            (uintptr_t) ndp,
            (uintptr_t) nhdp);
  }
  return ndp;
}

struct NaClDescDirDesc *NaClDescDirDescOpen(char *path) {
  struct NaClHostDir  *nhdp;

  nhdp = malloc(sizeof *nhdp);
  if (NULL == nhdp) {
    NaClLog(LOG_FATAL, "NaClDescDirDescOpen: no memory for %s\n", path);
  }
  if (!NaClHostDirOpen(nhdp, path)) {
    NaClLog(LOG_FATAL, "NaClDescDirDescOpen: NaClHostDirOpen failed for %s\n",
            path);
  }
  return NaClDescDirDescMake(nhdp);
}

static ssize_t NaClDescDirDescGetdents(struct NaClDesc         *vself,
                                       void                    *dirp,
                                       size_t                  count) {
  struct NaClDescDirDesc *self = (struct NaClDescDirDesc *) vself;

  return NaClHostDirGetdents(self->hd, dirp, count);
}

static ssize_t NaClDescDirDescRead(struct NaClDesc         *vself,
                                   void                    *buf,
                                   size_t                  len) {
  /* NaClLog(LOG_ERROR, "NaClDescDirDescRead: Read not allowed on dir\n"); */
  return NaClDescDirDescGetdents(vself, buf, len);
  /* return -NACL_ABI_EINVAL; */
}

static nacl_off64_t NaClDescDirDescSeek(struct NaClDesc *vself,
                                        nacl_off64_t    offset,
                                        int             whence) {
  struct NaClDescDirDesc *self = (struct NaClDescDirDesc *) vself;
  /* Special case to handle rewinddir() */
  if (offset == 0 || whence == SEEK_SET) {
    NaClHostDirRewind(self->hd);
    return 0;
  }
  return -NACL_ABI_EINVAL;
}

static int NaClDescDirDescFstat(struct NaClDesc          *vself,
                                struct nacl_abi_stat     *statbuf) {
  UNREFERENCED_PARAMETER(vself);

  memset(statbuf, 0, sizeof *statbuf);
  /*
   * TODO(bsy): saying it's executable/searchable might be a lie.
   */
  statbuf->nacl_abi_st_mode = (NACL_ABI_S_IFDIR |
                               NACL_ABI_S_IRUSR |
                               NACL_ABI_S_IXUSR);
  return 0;
}

static int NaClDescDirDescFchdir(struct NaClDesc *vself) {
  struct NaClDescDirDesc *self = (struct NaClDescDirDesc *) vself;

  return NaClHostDirFchdir(self->hd);
}

static int NaClDescDirDescFchmod(struct NaClDesc  *vself,
                                 int              mode) {
  struct NaClDescDirDesc *self = (struct NaClDescDirDesc *) vself;

  return NaClHostDirFchmod(self->hd, mode);
}

static int NaClDescDirDescFsync(struct NaClDesc *vself) {
  struct NaClDescDirDesc *self = (struct NaClDescDirDesc *) vself;

  return NaClHostDirFsync(self->hd);
}

static int NaClDescDirDescFdatasync(struct NaClDesc *vself) {
  struct NaClDescDirDesc *self = (struct NaClDescDirDesc *) vself;

  return NaClHostDirFdatasync(self->hd);
}

static struct NaClDescVtbl const kNaClDescDirDescVtbl = {
  {
    NaClDescDirDescDtor,
  },
  NaClDescMapNotImplemented,
  NaClDescDirDescRead,
  NaClDescWriteNotImplemented,
  NaClDescDirDescSeek,
  NaClDescPReadNotImplemented,
  NaClDescPWriteNotImplemented,
  NaClDescDirDescFstat,
  NaClDescDirDescFchdir,
  NaClDescDirDescFchmod,
  NaClDescDirDescFsync,
  NaClDescDirDescFdatasync,
  NaClDescFtruncateNotImplemented,
  NaClDescDirDescGetdents,
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
  NaClDescPostNotImplemented,
  NaClDescSemWaitNotImplemented,
  NaClDescGetValueNotImplemented,
  NaClDescSetMetadata,
  NaClDescGetMetadata,
  NaClDescSetFlags,
  NaClDescGetFlags,
  NaClDescIsattyNotImplemented,
  NACL_DESC_DIR,
};
