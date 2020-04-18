/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Descriptor quota management interface.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_QUOTA_INTERFACE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_QUOTA_INTERFACE_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/nacl_base/nacl_refcount.h"

EXTERN_C_BEGIN

struct NaClDescQuotaInterface;

/*
 * The virtual function table for NaClDescQuotaInterface.
 */

struct NaClDescQuotaInterfaceVtbl {
  struct NaClRefCountVtbl vbase;
  int64_t (*WriteRequest)(struct NaClDescQuotaInterface  *vself,
                          uint8_t const                  *file_id,
                          int64_t                        offset,
                          int64_t                        length) NACL_WUR;
  int64_t (*FtruncateRequest)(struct NaClDescQuotaInterface  *vself,
                              uint8_t const                  *file_id,
                              int64_t                        length)
      NACL_WUR;
};

struct NaClDescQuotaInterface {
  struct NaClRefCount base NACL_IS_REFCOUNT_SUBCLASS;
};

/*
 * Placement new style ctor; creates w/ ref_count of 1.
 *
 * The subclasses' ctor must call this base class ctor during their
 * contruction.
 */
int NaClDescQuotaInterfaceCtor(struct NaClDescQuotaInterface *ndqip) NACL_WUR;

extern struct NaClDescQuotaInterfaceVtbl const kNaClDescQuotaInterfaceVtbl;

struct NaClDescQuotaInterface *NaClDescQuotaInterfaceRef(
    struct NaClDescQuotaInterface *ndp);

/* when ref_count reaches zero, will call dtor and free */
void NaClDescQuotaInterfaceUnref(struct NaClDescQuotaInterface *ndqip);
/* as above, but works if ndqip is NULL. */
void NaClDescQuotaInterfaceSafeUnref(struct NaClDescQuotaInterface *ndqip);

int64_t NaClDescQuotaInterfaceWriteRequestNotImplemented(
    struct NaClDescQuotaInterface  *vself,
    uint8_t const                  *file_id,
    int64_t                        offset,
    int64_t                        length);

int64_t NaClDescQuotaInterfaceFtruncateRequestNotImplemented(
    struct NaClDescQuotaInterface  *vself,
    uint8_t const                  *file_id,
    int64_t                        length);

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_QUOTA_INTERFACE_H_
