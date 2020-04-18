/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor quota interface.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_platform.h"

#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/desc/nacl_desc_quota_interface.h"

#include "native_client/src/trusted/nacl_base/nacl_refcount.h"

int NaClDescQuotaInterfaceCtor(struct NaClDescQuotaInterface *self) {
  if (!NaClRefCountCtor(&self->base)) {
    return 0;
  }
  self->base.vtbl = (struct NaClRefCountVtbl *) (&kNaClDescQuotaInterfaceVtbl);
  return 1;
}

static void NaClDescQuotaInterfaceDtor(struct NaClRefCount *nrcp) {
  nrcp->vtbl = &kNaClRefCountVtbl;
  (*nrcp->vtbl->Dtor)(nrcp);
}

struct NaClDescQuotaInterface *NaClDescQuotaInterfaceRef(
    struct NaClDescQuotaInterface *ndqip) {
  return (struct NaClDescQuotaInterface *) NaClRefCountRef(&ndqip->base);
}

void NaClDescQuotaInterfaceUnref(struct NaClDescQuotaInterface *ndqip) {
  NaClRefCountUnref(&ndqip->base);
}

void NaClDescQuotaInterfaceSafeUnref(struct NaClDescQuotaInterface *ndqip) {
  if (NULL != ndqip) {
    NaClRefCountUnref(&ndqip->base);
  }
}

int64_t NaClDescQuotaInterfaceWriteRequestNotImplemented(
    struct NaClDescQuotaInterface  *vself,
    uint8_t const                  *file_id,
    int64_t                        offset,
    int64_t                        length) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(file_id);
  UNREFERENCED_PARAMETER(offset);
  UNREFERENCED_PARAMETER(length);
  NaClLog(LOG_FATAL, "NaClDescQuotaInterface: WriteRequest not implemented.");
  return 0;
}

int64_t NaClDescQuotaInterfaceFtruncateRequestNotImplemented(
    struct NaClDescQuotaInterface  *vself,
    uint8_t const                  *file_id,
    int64_t                        length) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(file_id);
  UNREFERENCED_PARAMETER(length);
  NaClLog(LOG_FATAL,
          "NaClDescQuotaInterface: FtruncateRequest not implemented.");
  return 0;
}

struct NaClDescQuotaInterfaceVtbl const kNaClDescQuotaInterfaceVtbl = {
  {
    NaClDescQuotaInterfaceDtor,
  },
  NaClDescQuotaInterfaceWriteRequestNotImplemented,
  NaClDescQuotaInterfaceFtruncateRequestNotImplemented,
};
