/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime.  NaClDescMutex subclass of NaClDesc.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_MUTEX_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_MUTEX_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"

#include "native_client/src/shared/platform/nacl_interruptible_mutex.h"

EXTERN_C_BEGIN

struct NaClDescEffector;
struct NaClDescXferState;

struct NaClDescMutex {
  struct NaClDesc      base NACL_IS_REFCOUNT_SUBCLASS;
  struct NaClIntrMutex mu;
};

int NaClDescMutexCtor(struct NaClDescMutex  *self) NACL_WUR;

struct NaClDescMutex *NaClDescMutexMake(struct NaClIntrMutex *mu);

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_MUTEX_H_
