/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime.  NaClDescCondVar subclass of NaClDesc.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_COND_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_COND_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_mutex.h"
#include "native_client/src/trusted/desc/nacl_desc_cond.h"

#include "native_client/src/shared/platform/nacl_interruptible_condvar.h"

EXTERN_C_BEGIN

struct NaClDescEffector;
struct NaClDescXferState;
struct NaClMessageHeader;
struct nacl_abi_timespec;

struct NaClDescCondVar {
  struct NaClDesc        base NACL_IS_REFCOUNT_SUBCLASS;
  struct NaClIntrCondVar cv;
};

int NaClDescCondVarCtor(struct NaClDescCondVar  *self)
    NACL_WUR;

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_COND_H_
