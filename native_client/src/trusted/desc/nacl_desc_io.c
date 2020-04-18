/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

#if NACL_WINDOWS
# include "io.h"
# include "fcntl.h"
#endif

#include <string.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"

#include "native_client/src/shared/platform/nacl_find_addrsp.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/internal_errno.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"


/*
 * This file contains the implementation for the NaClIoDesc subclass
 * of NaClDesc.
 *
 * NaClDescIoDesc is the subclass that wraps host-OS descriptors
 * provided by NaClHostDesc (which gives an OS-independent abstraction
 * for host-OS descriptors).
 */

static struct NaClDescVtbl const kNaClDescIoDescVtbl;  /* fwd */

static int NaClDescIoDescSubclassCtor(struct NaClDescIoDesc  *self,
                                      struct NaClHostDesc    *hd) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  self->hd = hd;
  basep->base.vtbl = (struct NaClRefCountVtbl const *) &kNaClDescIoDescVtbl;
  return 1;
}

/*
 * Takes ownership of hd, will close in Dtor.
 */
int NaClDescIoDescCtor(struct NaClDescIoDesc  *self,
                       struct NaClHostDesc    *hd) {
  struct NaClDesc *basep = (struct NaClDesc *) self;
  int rv;

  basep->base.vtbl = (struct NaClRefCountVtbl const *) NULL;
  if (!NaClDescCtor(basep)) {
    return 0;
  }
  rv = NaClDescIoDescSubclassCtor(self, hd);
  if (!rv) {
    (*NACL_VTBL(NaClRefCount, basep)->Dtor)((struct NaClRefCount *) basep);
  }
  (*NACL_VTBL(NaClDesc, basep)->
   SetFlags)(basep, hd->flags & NACL_ABI_O_ACCMODE);
  return rv;
}

static void NaClDescIoDescDtor(struct NaClRefCount *vself) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;

  NaClLog(4, "NaClDescIoDescDtor(0x%08"NACL_PRIxPTR").\n",
          (uintptr_t) vself);
  if (0 != NaClHostDescClose(self->hd)) {
    NaClLog(LOG_FATAL, "NaClDescIoDescDtor: NaClHostDescClose failed\n");
  }
  free(self->hd);
  self->hd = NULL;
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

struct NaClDescIoDesc *NaClDescIoDescMake(struct NaClHostDesc *nhdp) {
  struct NaClDescIoDesc *ndp;

  ndp = malloc(sizeof *ndp);
  if (NULL == ndp) {
    NaClLog(LOG_FATAL,
            "NaClDescIoDescMake: no memory for 0x%08"NACL_PRIxPTR"\n",
            (uintptr_t) nhdp);
  }
  if (!NaClDescIoDescCtor(ndp, nhdp)) {
    NaClLog(LOG_FATAL,
            ("NaClDescIoDescMake:"
             " NaClDescIoDescCtor(0x%08"NACL_PRIxPTR",0x%08"NACL_PRIxPTR
             ") failed\n"),
            (uintptr_t) ndp,
            (uintptr_t) nhdp);
  }
  return ndp;
}

struct NaClDesc *NaClDescIoMakeFromHandle(NaClHandle handle, int flags) {
  int posix_d;

#if NACL_WINDOWS
  int win_flags = 0;

  switch (flags & NACL_ABI_O_ACCMODE) {
    case NACL_ABI_O_RDONLY:
      win_flags = _O_RDONLY | _O_BINARY;
      break;
    case NACL_ABI_O_WRONLY:
      win_flags = _O_WRONLY | _O_BINARY;
      break;
    case NACL_ABI_O_RDWR:
      win_flags = _O_RDWR | _O_BINARY;
      break;
  }
  if (0 == win_flags) {
    return NULL;
  }
  posix_d = _open_osfhandle((intptr_t) handle, win_flags);
  if (-1 == posix_d) {
    return NULL;
  }
#else
  posix_d = handle;
#endif
  return NaClDescIoDescFromDescAllocCtor(posix_d, flags);
}

struct NaClDesc *NaClDescIoDescFromDescAllocCtor(int desc,
                                                 int flags) {
  struct NaClHostDesc *nhdp;

  nhdp = NaClHostDescPosixMake(desc, flags);
  if (NULL == nhdp) {
    /*
     * BUG: In Windows, we leak posix_d representation in the POSIX
     * layer, since caller will continue to own |handle| on a failure
     * return, but we cannot close |posix_d| without implicitly
     * invoking CloseHandle on |handle|.
     */
    return NULL;
  }
  return (struct NaClDesc *) NaClDescIoDescMake(nhdp);
}

struct NaClDescIoDesc *NaClDescIoDescOpen(char const *path,
                                          int mode,
                                          int perms) {
  struct NaClHostDesc *nhdp;

  nhdp = malloc(sizeof *nhdp);
  if (NULL == nhdp) {
    NaClLog(LOG_FATAL, "NaClDescIoDescOpen: no memory for %s\n", path);
  }
  if (0 != NaClHostDescOpen(nhdp, path, mode, perms)) {
    NaClLog(4,
            "NaClDescIoDescOpen: NaClHostDescOpen failed for %s\n",
            path);
    return NULL;
  }
  return NaClDescIoDescMake(nhdp);
}

static uintptr_t NaClDescIoDescMap(struct NaClDesc         *vself,
                                   struct NaClDescEffector *effp,
                                   void                    *start_addr,
                                   size_t                  len,
                                   int                     prot,
                                   int                     flags,
                                   nacl_off64_t            offset) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;
  uintptr_t             status;
  uintptr_t             addr;

  /*
   * prot must only contain NACL_ABI_PROT_* flags.
   */
  if (0 != (~(NACL_ABI_PROT_MASK) & prot)) {
    NaClLog(LOG_INFO,
            ("NaClDescIoDescMap: prot has other bits"
             " than NACL_ABI_PROT_{READ|WRITE|EXEC}\n"));
    return (uintptr_t) -NACL_ABI_EINVAL;
  }

  if (0 == (NACL_ABI_MAP_FIXED & flags)) {
    if (!NaClFindAddressSpace(&addr, len)) {
      NaClLog(1, "NaClDescIoDescMap: no address space?\n");
      return (uintptr_t) -NACL_ABI_ENOMEM;
    }
    NaClLog(4,
            "NaClDescIoDescMap: NaClFindAddressSpace"
            " returned 0x%"NACL_PRIxPTR"\n",
            addr);
    start_addr = (void *) addr;
  }
  flags |= NACL_ABI_MAP_FIXED;

  status = NaClHostDescMap((NULL == self) ? NULL : self->hd,
                           effp,
                           (void *) start_addr,
                           len,
                           prot,
                           flags,
                           offset);
  NaClLog(4, "NaClDescIoDescMap returning %"NACL_PRIxPTR"\n", status);
  return status;
}

uintptr_t NaClDescIoDescMapAnon(struct NaClDescEffector *effp,
                                void                    *start_addr,
                                size_t                  len,
                                int                     prot,
                                int                     flags,
                                nacl_off64_t            offset) {
  return NaClDescIoDescMap((struct NaClDesc *) NULL, effp, start_addr, len,
                           prot, flags, offset);
}

static ssize_t NaClDescIoDescRead(struct NaClDesc          *vself,
                                  void                     *buf,
                                  size_t                   len) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;

  return NaClHostDescRead(self->hd, buf, len);
}

static ssize_t NaClDescIoDescWrite(struct NaClDesc         *vself,
                                   void const              *buf,
                                   size_t                  len) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;

  return NaClHostDescWrite(self->hd, buf, len);
}

static nacl_off64_t NaClDescIoDescSeek(struct NaClDesc          *vself,
                                       nacl_off64_t             offset,
                                       int                      whence) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;

  return NaClHostDescSeek(self->hd, offset, whence);
}

static ssize_t NaClDescIoDescPRead(struct NaClDesc *vself,
                                   void *buf,
                                   size_t len,
                                   nacl_off64_t offset) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;

  return NaClHostDescPRead(self->hd, buf, len, offset);
}

static ssize_t NaClDescIoDescPWrite(struct NaClDesc *vself,
                                    void const *buf,
                                    size_t len,
                                    nacl_off64_t offset) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;

  return NaClHostDescPWrite(self->hd, buf, len, offset);
}

static int NaClDescIoDescFstat(struct NaClDesc         *vself,
                               struct nacl_abi_stat    *statbuf) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;
  int                   rv;
  nacl_host_stat_t      hstatbuf;

  rv = NaClHostDescFstat(self->hd, &hstatbuf);
  if (0 != rv) {
    return rv;
  }
  return NaClAbiStatHostDescStatXlateCtor(statbuf, &hstatbuf);
}

static int NaClDescIoDescFchmod(struct NaClDesc *vself,
                                int             mode) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;

  return NaClHostDescFchmod(self->hd, mode);
}

static int NaClDescIoDescFsync(struct NaClDesc *vself) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;

  return NaClHostDescFsync(self->hd);
}

static int NaClDescIoDescFdatasync(struct NaClDesc *vself) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;

  return NaClHostDescFdatasync(self->hd);
}

static int NaClDescIoDescFtruncate(struct NaClDesc  *vself,
                                   nacl_abi_off_t   length) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;

  return NaClHostDescFtruncate(self->hd, length);
}

static int32_t NaClDescIoIsatty(struct NaClDesc *vself) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;

  return NaClHostDescIsatty(self->hd);
}

static int NaClDescIoDescExternalizeSize(struct NaClDesc *vself,
                                         size_t          *nbytes,
                                         size_t          *nhandles) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;
  int rv;

  rv = NaClDescExternalizeSize(vself, nbytes, nhandles);
  if (0 != rv) {
    return rv;
  }
  *nhandles += 1;
  *nbytes += sizeof self->hd->flags;
  /* For Windows, we do not need to send flProtect since it is a cache */
  return 0;
}

static int NaClDescIoDescExternalize(struct NaClDesc           *vself,
                                     struct NaClDescXferState  *xfer) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;
  int rv;
#if NACL_WINDOWS
  HANDLE  h;
#endif

  rv = NaClDescExternalize(vself, xfer);
  if (0 != rv) {
    return rv;
  }

  memcpy(xfer->next_byte, &self->hd->flags, sizeof self->hd->flags);
  xfer->next_byte += sizeof self->hd->flags;
#if NACL_WINDOWS
  h = (HANDLE) _get_osfhandle(self->hd->d);
  *xfer->next_handle++ = (NaClHandle) h;
#else
  *xfer->next_handle++ = self->hd->d;
#endif
  return 0;
}

static struct NaClDescVtbl const kNaClDescIoDescVtbl = {
  {
    NaClDescIoDescDtor,
  },
  NaClDescIoDescMap,
  NaClDescIoDescRead,
  NaClDescIoDescWrite,
  NaClDescIoDescSeek,
  NaClDescIoDescPRead,
  NaClDescIoDescPWrite,
  NaClDescIoDescFstat,
  NaClDescFchdirNotImplemented,
  NaClDescIoDescFchmod,
  NaClDescIoDescFsync,
  NaClDescIoDescFdatasync,
  NaClDescIoDescFtruncate,
  NaClDescGetdentsNotImplemented,
  NaClDescIoDescExternalizeSize,
  NaClDescIoDescExternalize,
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
  NaClDescIoIsatty,
  NACL_DESC_HOST_IO,
};

/* set *out_desc to struct NaClDescIo * output */
int NaClDescIoInternalize(struct NaClDesc               **out_desc,
                          struct NaClDescXferState      *xfer) {
  int                   rv;
  NaClHandle            h;
  int                   d;
  int                   flags;
  struct NaClHostDesc   *nhdp;
  struct NaClDescIoDesc *ndidp;

  rv = -NACL_ABI_EIO;  /* catch-all */
  h = NACL_INVALID_HANDLE;
  nhdp = NULL;
  ndidp = NULL;

  nhdp = malloc(sizeof *nhdp);
  if (NULL == nhdp) {
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }
  ndidp = malloc(sizeof *ndidp);
  if (!ndidp) {
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }
  if (!NaClDescInternalizeCtor((struct NaClDesc *) ndidp, xfer)) {
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }
  if (xfer->next_handle == xfer->handle_buffer_end ||
      xfer->next_byte + sizeof ndidp->hd->flags > xfer->byte_buffer_end) {
    rv = -NACL_ABI_EIO;
    goto cleanup_ndidp_dtor;
  }

  NACL_COMPILE_TIME_ASSERT(sizeof flags == sizeof(ndidp->hd->flags));
  memcpy(&flags, xfer->next_byte, sizeof flags);
  xfer->next_byte += sizeof flags;

  h = *xfer->next_handle;
  *xfer->next_handle++ = NACL_INVALID_HANDLE;
#if NACL_WINDOWS
  if (-1 == (d = _open_osfhandle((intptr_t) h, _O_RDWR | _O_BINARY))) {
    rv = -NACL_ABI_EIO;
    goto cleanup_ndidp_dtor;
  }
#else
  d = h;
#endif
  /*
   * We mark it as read/write, but don't really know for sure until we
   * try to make those syscalls (in which case we'd get EBADF).
   */
  if ((rv = NaClHostDescPosixTake(nhdp, d, flags)) < 0) {
    goto cleanup_ndidp_dtor;
  }
  h = NACL_INVALID_HANDLE;  /* nhdp took ownership of h */

  if (!NaClDescIoDescSubclassCtor(ndidp, nhdp)) {
    rv = -NACL_ABI_ENOMEM;
    goto cleanup_nhdp_dtor;
  }
  /*
   * ndidp took ownership of nhdp, now give ownership of ndidp to caller.
   */
  *out_desc = (struct NaClDesc *) ndidp;
  rv = 0;
 cleanup_nhdp_dtor:
  if (rv < 0) {
    if (0 != NaClHostDescClose(nhdp)) {
      NaClLog(LOG_FATAL, "NaClDescIoInternalize: NaClHostDescClose failed\n");
    }
  }
 cleanup_ndidp_dtor:
  if (rv < 0) {
    NaClDescSafeUnref((struct NaClDesc *) ndidp);
    ndidp = NULL;
  }
 cleanup:
  if (rv < 0) {
    free(nhdp);
    free(ndidp);
    if (NACL_INVALID_HANDLE != h) {
      (void) NaClClose(h);
    }
  }
  return rv;
}
