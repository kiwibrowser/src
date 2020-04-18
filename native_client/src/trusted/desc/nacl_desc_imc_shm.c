/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Transferrable shared memory objects.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_platform.h"

#include <stdlib.h>
#include <string.h>

#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_shm.h"

#include "native_client/src/shared/platform/nacl_find_addrsp.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/internal_errno.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * This file contains the implementation of the NaClDescImcShm
 * subclass of NaClDesc.
 *
 * NaClDescImcShm is the subclass that wraps IMC shm descriptors.
 */

static struct NaClDescVtbl const kNaClDescImcShmVtbl;  /* fwd */

static int NaClDescImcShmSubclassCtor(struct NaClDescImcShm  *self,
                                      NaClHandle             h,
                                      nacl_off64_t           size) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  /*
   * off_t is signed, but size_t are not; historically size_t is for
   * sizeof and similar, and off_t is also used for stat structure
   * st_size member.  This runtime test detects large object sizes
   * that are silently converted to negative values.
   */
  if (size < 0 || SIZE_T_MAX < (uint64_t) size) {
    return 0;
  }
  self->h = h;
  self->size = size;
  basep->base.vtbl = (struct NaClRefCountVtbl const *) &kNaClDescImcShmVtbl;
  return 1;
}

int NaClDescImcShmCtor(struct NaClDescImcShm  *self,
                       NaClHandle             h,
                       nacl_off64_t           size) {
  struct NaClDesc *basep = (struct NaClDesc *) self;
  int rv;

  basep->base.vtbl = (struct NaClRefCountVtbl const *) NULL;

  if (!NaClDescCtor(basep)) {
    return 0;
  }
  rv = NaClDescImcShmSubclassCtor(self, h, size);
  if (!rv) {
    /* NaClDescImcShm construction failed, still a NaClDesc object */
    (*NACL_VTBL(NaClRefCount, basep)->Dtor)((struct NaClRefCount *) basep);
  }
  (*NACL_VTBL(NaClDesc, basep)->SetFlags)(basep, NACL_ABI_O_RDWR);
  return 1;
}

int NaClDescImcShmAllocCtor(struct NaClDescImcShm  *self,
                            nacl_off64_t           size,
                            int                    executable) {
  NaClHandle h;
  int        rv;

  if (size < 0 || SIZE_T_MAX < (uint64_t) size) {
    NaClLog(4,
            "NaClDescImcShmAllocCtor: requested size 0x%08"NACL_PRIx64
            " (0x%08"NACL_PRId64") too large\n",
            size, size);
    return 0;
  }
  h = NaClCreateMemoryObject((size_t) size, executable);
  if (NACL_INVALID_HANDLE == h) {
    return 0;
  }
  if (0 == (rv = NaClDescImcShmCtor(self, h, size))) {
    (void) NaClClose(h);
  }
  return rv;
}

struct NaClDesc *NaClDescImcShmMake(NaClHandle handle, nacl_off64_t size) {
  struct NaClDescImcShm *desc = malloc(sizeof(*desc));
  if (NULL == desc) {
    return NULL;
  }
  if (!NaClDescImcShmCtor(desc, handle, size)) {
    free(desc);
    return NULL;
  }
  return &desc->base;
}

static void NaClDescImcShmDtor(struct NaClRefCount *vself) {
  struct NaClDescImcShm  *self = (struct NaClDescImcShm *) vself;

  (void) NaClClose(self->h);
  self->h = NACL_INVALID_HANDLE;
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

static uintptr_t NaClDescImcShmMap(struct NaClDesc         *vself,
                                   struct NaClDescEffector *effp,
                                   void                    *start_addr,
                                   size_t                  len,
                                   int                     prot,
                                   int                     flags,
                                   nacl_off64_t            offset) {
  struct NaClDescImcShm  *self = (struct NaClDescImcShm *) vself;

  uintptr_t     addr;
  void          *result;
  nacl_off64_t  tmp_off64;

  /*
   * shm must have NACL_ABI_MAP_SHARED in flags, and all calls through
   * this API must supply a start_addr, so NACL_ABI_MAP_FIXED is
   * assumed.
   */
  if (NACL_ABI_MAP_SHARED != (flags & NACL_ABI_MAP_SHARING_MASK)) {
    NaClLog(LOG_INFO,
            ("NaClDescImcShmMap: Mapping not NACL_ABI_MAP_SHARED,"
             " flags 0x%x\n"),
            flags);
    return (uintptr_t) -NACL_ABI_EINVAL;
  }
  if (0 != (NACL_ABI_MAP_FIXED & flags) && NULL == start_addr) {
    NaClLog(LOG_INFO,
            ("NaClDescImcShmMap: Mapping NACL_ABI_MAP_FIXED"
             " but start_addr is NULL\n"));
  }
  /* post-condition: if NULL == start_addr, then NACL_ABI_MAP_FIXED not set */

  /*
   * prot must not contain bits other than PROT_{READ|WRITE|EXEC}.
   */
  if (0 != (~(NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE | NACL_ABI_PROT_EXEC)
            & prot)) {
    NaClLog(LOG_INFO,
            "NaClDescImcShmMap: prot has other bits than"
            " PROT_{READ|WRITE|EXEC}\n");
    return (uintptr_t) -NACL_ABI_EINVAL;
  }
  /*
   * Map from NACL_ABI_ prot and flags bits to IMC library flags,
   * which will later map back into posix-style prot/flags on *x
   * boxen, and to MapViewOfFileEx arguments on Windows.
   */
  if (0 == (NACL_ABI_MAP_FIXED & flags)) {
    /* start_addr is a hint, and we just ignore the hint... */
    if (!NaClFindAddressSpace(&addr, len)) {
      NaClLog(1, "NaClDescImcShmMap: no address space?!?\n");
      return (uintptr_t) -NACL_ABI_ENOMEM;
    }
    start_addr = (void *) addr;
  }

  tmp_off64 = offset + len;
  /* just NaClRoundAllocPage, but in 64 bits */
  tmp_off64 = ((tmp_off64 + NACL_MAP_PAGESIZE - 1)
             & ~(uint64_t) (NACL_MAP_PAGESIZE - 1));
  if (tmp_off64 > INT32_MAX) {
    NaClLog(LOG_INFO,
            "NaClDescImcShmMap: total offset exceeds 32-bits\n");
    return (uintptr_t) -NACL_ABI_EOVERFLOW;
  }

  result = NaClMap(effp,
                   (void *) start_addr,
                   len,
                   prot,
                   NACL_ABI_MAP_SHARED | NACL_ABI_MAP_FIXED,
                   self->h,
                   (off_t) offset);
  if (NACL_ABI_MAP_FAILED == result) {
    return (uintptr_t) -NACL_ABI_E_MOVE_ADDRESS_SPACE;
  }
  if (0 != (NACL_ABI_MAP_FIXED & flags) && result != (void *) start_addr) {
    NaClLog(LOG_FATAL,
            "NaClDescImcShmMap: NACL_ABI_MAP_FIXED but got %p instead of %p\n",
            result, start_addr);
  }
  return (uintptr_t) start_addr;
}

static int NaClDescImcShmFstat(struct NaClDesc         *vself,
                               struct nacl_abi_stat    *stbp) {
  struct NaClDescImcShm  *self = (struct NaClDescImcShm *) vself;

  if (self->size > INT32_MAX) {
    return -NACL_ABI_EOVERFLOW;
  }

  stbp->nacl_abi_st_dev = 0;
  stbp->nacl_abi_st_ino = NACL_FAKE_INODE_NUM;
  stbp->nacl_abi_st_mode = (NACL_ABI_S_IFSHM |
                            NACL_ABI_S_IRUSR |
                            NACL_ABI_S_IWUSR);
  stbp->nacl_abi_st_nlink = 1;
  stbp->nacl_abi_st_uid = (nacl_abi_uid_t) -1;
  stbp->nacl_abi_st_gid = (nacl_abi_gid_t) -1;
  stbp->nacl_abi_st_rdev = 0;
  stbp->nacl_abi_st_size = (nacl_abi_off_t) self->size;
  stbp->nacl_abi_st_blksize = 0;
  stbp->nacl_abi_st_blocks = 0;
  stbp->nacl_abi_st_atime = 0;
  stbp->nacl_abi_st_mtime = 0;
  stbp->nacl_abi_st_ctime = 0;

  return 0;
}

static int NaClDescImcShmExternalizeSize(struct NaClDesc *vself,
                                         size_t          *nbytes,
                                         size_t          *nhandles) {
  struct NaClDescImcShm  *self = (struct NaClDescImcShm *) vself;
  int rv;

  rv = NaClDescExternalizeSize(vself, nbytes, nhandles);
  if (0 != rv) {
    return rv;
  }
  *nbytes += sizeof self->size;
  *nhandles += 1;

  return 0;
}

static int NaClDescImcShmExternalize(struct NaClDesc           *vself,
                                     struct NaClDescXferState  *xfer) {
  struct NaClDescImcShm  *self = (struct NaClDescImcShm *) vself;
  int rv;

  rv = NaClDescExternalize(vself, xfer);
  if (0 != rv) {
    return rv;
  }
  *xfer->next_handle++ = self->h;
  memcpy(xfer->next_byte, &self->size, sizeof self->size);
  xfer->next_byte += sizeof self->size;
  return 0;
}

static struct NaClDescVtbl const kNaClDescImcShmVtbl = {
  {
    NaClDescImcShmDtor,
  },
  NaClDescImcShmMap,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescPReadNotImplemented,
  NaClDescPWriteNotImplemented,
  NaClDescImcShmFstat,
  NaClDescFchdirNotImplemented,
  NaClDescFchmodNotImplemented,
  NaClDescFsyncNotImplemented,
  NaClDescFdatasyncNotImplemented,
  NaClDescFtruncateNotImplemented,
  NaClDescGetdentsNotImplemented,
  NaClDescImcShmExternalizeSize,
  NaClDescImcShmExternalize,
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
  NACL_DESC_SHM,
};

int NaClDescImcShmInternalize(struct NaClDesc               **out_desc,
                              struct NaClDescXferState      *xfer) {
  int                   rv;
  struct NaClDescImcShm *ndisp;
  NaClHandle            h;
  nacl_off64_t          hsize;

  rv = -NACL_ABI_EIO;

  ndisp = malloc(sizeof *ndisp);
  if (NULL == ndisp) {
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }
  if (!NaClDescInternalizeCtor((struct NaClDesc *) ndisp, xfer)) {
    free(ndisp);
    ndisp = NULL;
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }

  if (xfer->next_handle == xfer->handle_buffer_end) {
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }
  if (xfer->next_byte + sizeof ndisp->size > xfer->byte_buffer_end) {
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }

  h = *xfer->next_handle;
  *xfer->next_handle++ = NACL_INVALID_HANDLE;
  memcpy(&hsize, xfer->next_byte, sizeof hsize);
  xfer->next_byte += sizeof hsize;

  if (!NaClDescImcShmSubclassCtor(ndisp, h, hsize)) {
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }

  *out_desc = (struct NaClDesc *) ndisp;
  rv = 0;

cleanup:
  if (rv < 0) {
    NaClDescSafeUnref((struct NaClDesc *) ndisp);
  }
  return rv;
}
