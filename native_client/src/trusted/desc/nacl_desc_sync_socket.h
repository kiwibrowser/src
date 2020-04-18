/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime.  NaClDescSyncSocket subclass of NaClDesc.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_SYNC_SOCKET_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_SYNC_SOCKET_H_

#include "native_client/src/include/portability.h"

#include "native_client/src/include/nacl_base.h"

#include "native_client/src/public/nacl_desc.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"

/*
 * get NaClHandle, which is a typedef and not a struct pointer, so
 * impossible to just forward declare.
 */
#include "native_client/src/shared/imc/nacl_imc_c.h"

EXTERN_C_BEGIN

struct NaClDescEffector;
struct NaClDescXferState;
struct NaClMessageHeader;

struct NaClDescSyncSocket {
  struct NaClDesc           base NACL_IS_REFCOUNT_SUBCLASS;
  NaClHandle                h;
};

int NaClDescSyncSocketInternalize(
    struct NaClDesc               **baseptr,
    struct NaClDescXferState      *xfer)
    NACL_WUR;

static const size_t kMaxSyncSocketMessageLength = (size_t) INT_MAX;

/* On success, NaClDescSyncSocket has ownership of h. */
int NaClDescSyncSocketCtor(struct NaClDescSyncSocket  *self,
                           NaClHandle                 h)
    NACL_WUR;

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_SYNC_SOCKET_H_
