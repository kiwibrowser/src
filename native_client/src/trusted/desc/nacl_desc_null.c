/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * A NaClDesc subclass that exposes a /dev/null interface.
 */

#include "native_client/src/trusted/desc/nacl_desc_null.h"

#include <string.h>

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

static struct NaClDescVtbl const kNaClDescNullVtbl;  /* fwd */

int NaClDescNullCtor(struct NaClDescNull *self) {
  if (!NaClDescCtor((struct NaClDesc *) self)) {
    return 0;
  }
  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl *) &kNaClDescNullVtbl;
  return 1;
}

static void NaClDescNullDtor(struct NaClRefCount *vself) {
  struct NaClDescNull *self = (struct NaClDescNull *) vself;

  NACL_VTBL(NaClDesc, self) = &kNaClDescVtbl;
  (*NACL_VTBL(NaClRefCount, self)->Dtor)((struct NaClRefCount *) self);
}

static ssize_t NaClDescNullRead(struct NaClDesc *vself,
                               void *buf,
                               size_t len) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(buf);
  UNREFERENCED_PARAMETER(len);

  return 0;
}

static ssize_t NaClDescNullWrite(struct NaClDesc *vself,
                                void const *buf,
                                size_t len) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(buf);

  return len;
}

static ssize_t NaClDescNullPRead(struct NaClDesc *vself,
                                 void *buf,
                                 size_t len,
                                 nacl_off64_t offset) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(buf);
  UNREFERENCED_PARAMETER(len);
  if (offset < 0) {
    return -NACL_ABI_EINVAL;
  }

  return 0;
}

static ssize_t NaClDescNullPWrite(struct NaClDesc *vself,
                                  void const *buf,
                                  size_t len,
                                  nacl_off64_t offset) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(buf);
  if (offset < 0) {
    return -NACL_ABI_EINVAL;
  }

  return len;
}

static int NaClDescNullFstat(struct NaClDesc *vself,
                            struct nacl_abi_stat *statbuf) {
  UNREFERENCED_PARAMETER(vself);

  memset(statbuf, 0, sizeof *statbuf);
  statbuf->nacl_abi_st_dev = 0;
  statbuf->nacl_abi_st_ino = NACL_FAKE_INODE_NUM;
  statbuf->nacl_abi_st_mode = NACL_ABI_S_IRUSR | NACL_ABI_S_IFCHR;
  statbuf->nacl_abi_st_nlink = 1;
  statbuf->nacl_abi_st_uid = (nacl_abi_uid_t) -1;
  statbuf->nacl_abi_st_gid = (nacl_abi_gid_t) -1;
  statbuf->nacl_abi_st_rdev = 0;
  statbuf->nacl_abi_st_size = 0;
  statbuf->nacl_abi_st_blksize = 0;
  statbuf->nacl_abi_st_blocks = 0;
  statbuf->nacl_abi_st_atime = 0;
  statbuf->nacl_abi_st_atimensec = 0;
  statbuf->nacl_abi_st_mtime = 0;
  statbuf->nacl_abi_st_mtimensec = 0;
  statbuf->nacl_abi_st_ctime = 0;
  statbuf->nacl_abi_st_ctimensec = 0;

  return 0;
}

/*
 * We allow descriptor "transfer", where in reality we create a
 * separate null device locally at the recipient end.  Read/write
 * operations on /dev/null are allowed, but does not make use of flags
 * nor are there opportunity to use metadata, so like the invalid
 * descriptor, we don't bother to transfer that data.
 */
static int NaClDescNullExternalizeSize(struct NaClDesc *vself,
                                       size_t *nbytes,
                                       size_t *nhandles) {
  UNREFERENCED_PARAMETER(vself);

  *nbytes = 0;
  *nhandles = 0;
  return 0;
}

static int NaClDescNullExternalize(struct NaClDesc *vself,
                                   struct NaClDescXferState *xfer) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(xfer);

  return 0;
}

static struct NaClDescVtbl const kNaClDescNullVtbl = {
  {
    NaClDescNullDtor,
  },
  NaClDescMapNotImplemented,
  NaClDescNullRead,
  NaClDescNullWrite,
  NaClDescSeekNotImplemented,
  NaClDescNullPRead,
  NaClDescNullPWrite,
  NaClDescNullFstat,
  NaClDescFchdirNotImplemented,
  NaClDescFchmodNotImplemented,
  NaClDescFsyncNotImplemented,
  NaClDescFdatasyncNotImplemented,
  NaClDescFtruncateNotImplemented,
  NaClDescGetdentsNotImplemented,
  NaClDescNullExternalizeSize,
  NaClDescNullExternalize,
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
  NACL_DESC_NULL,
};

int NaClDescNullInternalize(struct NaClDesc **out_desc,
                            struct NaClDescXferState *xfer) {
  int rv;
  struct NaClDescNull *d_null = malloc(sizeof *d_null);

  UNREFERENCED_PARAMETER(xfer);
  if (NULL == d_null) {
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }
  if (!NaClDescNullCtor(d_null)) {
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }
  *out_desc = (struct NaClDesc *) d_null;
  rv = 0;  /* yay! */
 cleanup:
  return rv;
}
