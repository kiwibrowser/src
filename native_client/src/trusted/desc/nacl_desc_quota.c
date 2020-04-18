/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/desc/nacl_desc_quota.h"
#include "native_client/src/trusted/desc/nacl_desc_quota_interface.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nrd_xfer_intern.h"
#include "native_client/src/trusted/nacl_base/nacl_refcount.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"

static struct NaClDescVtbl const kNaClDescQuotaVtbl;

int NaClDescQuotaSubclassCtor(struct NaClDescQuota           *self,
                              struct NaClDesc                *desc,
                              uint8_t const                  *file_id,
                              struct NaClDescQuotaInterface  *quota_interface) {
  if (!NaClMutexCtor(&self->mu)) {
    /* do not NaClRefCountUnref, since we cannot free: caller must do that */
    (*NACL_VTBL(NaClRefCount, self)->Dtor)((struct NaClRefCount *) self);
    return 0;
  }
  self->desc = desc;  /* take ownership */
  memcpy(self->file_id, file_id, NACL_DESC_QUOTA_FILE_ID_LEN);
  if (NULL == quota_interface) {
    self->quota_interface = (struct NaClDescQuotaInterface *) NULL;
  } else {
    self->quota_interface = NaClDescQuotaInterfaceRef(quota_interface);
  }
  NACL_VTBL(NaClDesc, self) = &kNaClDescQuotaVtbl;
  return 1;
}

int NaClDescQuotaCtor(struct NaClDescQuota           *self,
                      struct NaClDesc                *desc,
                      uint8_t const                  *file_id,
                      struct NaClDescQuotaInterface  *quota_interface) {
  int rv;
  if (!NaClDescCtor(&self->base)) {
    NACL_VTBL(NaClDesc, self) = NULL;
    return 0;
  }
  rv = NaClDescQuotaSubclassCtor(self, desc, file_id, quota_interface);
  if (!rv) {
    (*NACL_VTBL(NaClRefCount, self)->Dtor)((struct NaClRefCount *) self);
    return 0;
  }
  return rv;
}

void NaClDescQuotaDtor(struct NaClRefCount *vself) {
  struct NaClDescQuota *self = (struct NaClDescQuota *) vself;

  NaClRefCountSafeUnref((struct NaClRefCount *) self->quota_interface);
  NaClRefCountUnref((struct NaClRefCount *) self->desc);
  self->desc = NULL;
  NaClMutexDtor(&self->mu);

  NACL_VTBL(NaClDesc, self) = &kNaClDescVtbl;
  (*NACL_VTBL(NaClRefCount, self)->Dtor)(vself);
}

uintptr_t NaClDescQuotaMap(struct NaClDesc          *vself,
                           struct NaClDescEffector  *effp,
                           void                     *start_addr,
                           size_t                   len,
                           int                      prot,
                           int                      flags,
                           nacl_off64_t             offset) {
  struct NaClDescQuota *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->
          Map)(self->desc, effp, start_addr, len, prot, flags, offset);
}

ssize_t NaClDescQuotaRead(struct NaClDesc *vself,
                          void            *buf,
                          size_t          len) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Read)(self->desc, buf, len);
}

/*
 * Cap the length of a write/pwrite to a safe length.  Write can
 * always return a short, non-zero transfer count, so it's fine if
 * the amount actually processed here is less than that requested.
 * The quota APIs use int64_t for their length parameters, so the
 * safe limit INT64_MAX.  But on 64-bit machines, size_t might go up
 * to UINT64_MAX, which is too big.  This logic is a no-op for
 * 32-bit machines and the compiler should just optimize it away.
 * But the 32-bit Windows compiler complains about the code, so we
 * #if it out when it's not needed.
 */
static size_t CapWriteLength(size_t len) {
#if NACL_BUILD_SUBARCH == 64
  uint64_t len_u64 = (uint64_t) len;
  NACL_COMPILE_TIME_ASSERT(SIZE_T_MAX <= NACL_UMAX_VAL(uint64_t));
  if (len_u64 > NACL_MAX_VAL(int64_t)) {
    len = (size_t) NACL_MAX_VAL(int64_t);
  }
#endif
  return len;
}

ssize_t NaClDescQuotaWrite(struct NaClDesc  *vself,
                           void const       *buf,
                           size_t           len) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;
  nacl_off64_t          file_offset;
  int64_t               allowed;
  ssize_t               rv;

  NaClXMutexLock(&self->mu);
  if (0 == len) {
    allowed = 0;
  } else {
    /*
     * prevent another thread from doing a repositioning seek between the
     * lseek(d,0,1) and the Write.
     */
    file_offset = (*NACL_VTBL(NaClDesc, self->desc)->Seek)(self->desc,
                                                           0,
                                                           SEEK_CUR);
    if (file_offset < 0) {
      rv = (ssize_t) file_offset;
      goto abort;
    }

    len = CapWriteLength(len);

    if (NULL == self->quota_interface) {
      /* If there is no quota_interface, do not allow writes. */
      allowed = 0;
    } else {
      allowed = (*NACL_VTBL(NaClDescQuotaInterface, self->quota_interface)->
                 WriteRequest)(self->quota_interface,
                               self->file_id, file_offset, len);
    }
    if (allowed <= 0) {
      rv = -NACL_ABI_EDQUOT;
      goto abort;
    }
    /*
     * allowed <= len should be a post-condition, but we check for
     * it anyway.
     */
    if ((uint64_t) allowed > len) {
      NaClLog(LOG_WARNING,
              ("NaClDescQuotaWrite: WriteRequest returned an allowed quota"
               " that is larger than that requested; reducing to original"
               " request amount.\n"));
      allowed = len;
    }
  }

  /*
   * It is possible for Write to write fewer than bytes than the quota
   * that was granted, in which case quota will leak.
   * TODO(sehr,bsy): eliminate quota leakage.
   */
  rv = (*NACL_VTBL(NaClDesc, self->desc)->Write)(self->desc,
                                                 buf, (size_t) allowed);
abort:
  NaClXMutexUnlock(&self->mu);
  return rv;
}

ssize_t NaClDescQuotaPRead(struct NaClDesc *vself,
                           void *buf,
                           size_t len,
                           nacl_off64_t offset) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->PRead)(self->desc, buf, len,
                                                   offset);
}

ssize_t NaClDescQuotaPWrite(struct NaClDesc *vself,
                            void const *buf,
                            size_t len,
                            nacl_off64_t offset) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;
  int64_t               allowed;
  ssize_t               rv;

  if (0 == len) {
    allowed = 0;
  } else {
    len = CapWriteLength(len);

    if (NULL == self->quota_interface) {
      /* If there is no quota_interface, do not allow writes. */
      allowed = 0;
    } else {
      allowed = (*NACL_VTBL(NaClDescQuotaInterface, self->quota_interface)->
                 WriteRequest)(self->quota_interface,
                               self->file_id, offset, len);
    }
    if (allowed <= 0) {
      rv = -NACL_ABI_EDQUOT;
      goto abort;
    }
    /*
     * allowed <= len should be a post-condition, but we check for
     * it anyway.
     */
    if ((uint64_t) allowed > len) {
      NaClLog(LOG_WARNING,
              ("NaClDescQuotaPWrite: WriteRequest returned an allowed quota"
               " that is larger than that requested; reducing to original"
               " request amount.\n"));
      allowed = len;
    }
  }

  /*
   * It is possible for Write to write fewer than bytes than the quota
   * that was granted, in which case quota will leak.
   * TODO(sehr,bsy): eliminate quota leakage.
   */
  rv = (*NACL_VTBL(NaClDesc, self->desc)->PWrite)(self->desc,
                                                  buf, (size_t) allowed,
                                                  offset);
abort:
  return rv;
}

nacl_off64_t NaClDescQuotaSeek(struct NaClDesc  *vself,
                               nacl_off64_t     offset,
                               int              whence) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;
  nacl_off64_t          rv;

  NaClXMutexLock(&self->mu);
  rv = (*NACL_VTBL(NaClDesc, self->desc)->Seek)(self->desc, offset, whence);
  NaClXMutexUnlock(&self->mu);

  return rv;
}

int NaClDescQuotaFstat(struct NaClDesc      *vself,
                       struct nacl_abi_stat *statbuf) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Fstat)(self->desc, statbuf);
}

int NaClDescQuotaFchdir(struct NaClDesc *vself) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Fchdir)(self->desc);
}

int NaClDescQuotaFchmod(struct NaClDesc *vself,
                        int             mode) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Fchmod)(self->desc, mode);
}

int NaClDescQuotaFsync(struct NaClDesc *vself) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Fsync)(self->desc);
}

int NaClDescQuotaFdatasync(struct NaClDesc *vself) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Fdatasync)(self->desc);
}

int NaClDescQuotaFtruncate(struct NaClDesc  *vself,
                           nacl_abi_off_t   length) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Ftruncate)(self->desc, length);
}

ssize_t NaClDescQuotaGetdents(struct NaClDesc *vself,
                              void            *dirp,
                              size_t          count) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Getdents)(self->desc, dirp, count);
}

int NaClDescQuotaLock(struct NaClDesc *vself) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Lock)(self->desc);
}

int NaClDescQuotaTryLock(struct NaClDesc *vself) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->TryLock)(self->desc);
}

int NaClDescQuotaUnlock(struct NaClDesc *vself) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Unlock)(self->desc);
}

int NaClDescQuotaWait(struct NaClDesc *vself,
                      struct NaClDesc *mutex) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Wait)(self->desc, mutex);
}

int NaClDescQuotaTimedWaitAbs(struct NaClDesc                *vself,
                              struct NaClDesc                *mutex,
                              struct nacl_abi_timespec const *ts) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->TimedWaitAbs)(self->desc, mutex,
                                                          ts);
}

int NaClDescQuotaSignal(struct NaClDesc *vself) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Signal)(self->desc);
}

int NaClDescQuotaBroadcast(struct NaClDesc *vself) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Broadcast)(self->desc);
}

ssize_t NaClDescQuotaSendMsg(struct NaClDesc                 *vself,
                             const struct NaClImcTypedMsgHdr *nitmhp,
                             int                             flags) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->SendMsg)(self->desc, nitmhp, flags);
}

ssize_t NaClDescQuotaRecvMsg(struct NaClDesc               *vself,
                             struct NaClImcTypedMsgHdr     *nitmhp,
                             int                           flags) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->RecvMsg)(self->desc, nitmhp, flags);
}

ssize_t NaClDescQuotaLowLevelSendMsg(struct NaClDesc                *vself,
                                     struct NaClMessageHeader const *dgram,
                                     int                            flags) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->LowLevelSendMsg)(
      self->desc, dgram, flags);
}

ssize_t NaClDescQuotaLowLevelRecvMsg(struct NaClDesc           *vself,
                                     struct NaClMessageHeader  *dgram,
                                     int                       flags) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->LowLevelRecvMsg)(
      self->desc, dgram, flags);
}

int NaClDescQuotaConnectAddr(struct NaClDesc *vself,
                             struct NaClDesc **result) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->ConnectAddr)(self->desc, result);
}

int NaClDescQuotaAcceptConn(struct NaClDesc *vself,
                            struct NaClDesc **result) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->AcceptConn)(self->desc, result);
}

int NaClDescQuotaPost(struct NaClDesc *vself) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Post)(self->desc);
}

int NaClDescQuotaSemWait(struct NaClDesc *vself) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->SemWait)(self->desc);
}

int NaClDescQuotaGetValue(struct NaClDesc *vself) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->GetValue)(self->desc);
}

int NaClDescQuotaSetMetadata(struct NaClDesc *vself,
                             int32_t metadata_type,
                             uint32_t metadata_num_bytes,
                             uint8_t const *metadata_bytes) {
  struct NaClDescQuota *self = (struct NaClDescQuota *) vself;
  return (*NACL_VTBL(NaClDesc, self->desc)->SetMetadata)(self->desc,
                                                         metadata_type,
                                                         metadata_num_bytes,
                                                         metadata_bytes);
}

int32_t NaClDescQuotaGetMetadata(struct NaClDesc *vself,
                                 uint32_t *metadata_buffer_bytes_in_out,
                                 uint8_t *metadata_buffer) {
  struct NaClDescQuota *self = (struct NaClDescQuota *) vself;
  return (*NACL_VTBL(NaClDesc,
                     self->desc)->GetMetadata)(self->desc,
                                               metadata_buffer_bytes_in_out,
                                               metadata_buffer);
}

void NaClDescQuotaSetFlags(struct NaClDesc *vself,
                           uint32_t flags) {
  struct NaClDescQuota *self = (struct NaClDescQuota *) vself;
  (*NACL_VTBL(NaClDesc, self->desc)->SetFlags)(self->desc, flags);
}

uint32_t NaClDescQuotaGetFlags(struct NaClDesc *vself) {
  struct NaClDescQuota *self = (struct NaClDescQuota *) vself;
  return (*NACL_VTBL(NaClDesc, self->desc)->GetFlags)(self->desc);
}

static struct NaClDescVtbl const kNaClDescQuotaVtbl = {
  {
    NaClDescQuotaDtor,
  },
  NaClDescQuotaMap,
  NaClDescQuotaRead,
  NaClDescQuotaWrite,
  NaClDescQuotaSeek,
  NaClDescQuotaPRead,
  NaClDescQuotaPWrite,
  NaClDescQuotaFstat,
  NaClDescQuotaFchdir,
  NaClDescQuotaFchmod,
  NaClDescQuotaFsync,
  NaClDescQuotaFdatasync,
  NaClDescQuotaFtruncate,
  NaClDescQuotaGetdents,
  NaClDescExternalizeSizeNotImplemented,
  NaClDescExternalizeNotImplemented,
  NaClDescQuotaLock,
  NaClDescQuotaTryLock,
  NaClDescQuotaUnlock,
  NaClDescQuotaWait,
  NaClDescQuotaTimedWaitAbs,
  NaClDescQuotaSignal,
  NaClDescQuotaBroadcast,
  NaClDescQuotaSendMsg,
  NaClDescQuotaRecvMsg,
  NaClDescQuotaLowLevelSendMsg,
  NaClDescQuotaLowLevelRecvMsg,
  NaClDescQuotaConnectAddr,
  NaClDescQuotaAcceptConn,
  NaClDescQuotaPost,
  NaClDescQuotaSemWait,
  NaClDescQuotaGetValue,
  NaClDescQuotaSetMetadata,
  NaClDescQuotaGetMetadata,
  NaClDescQuotaSetFlags,
  NaClDescQuotaGetFlags,
  NaClDescIsattyNotImplemented,
  NACL_DESC_QUOTA,
};
