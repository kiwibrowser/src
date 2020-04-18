/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime.  NaClDescSemaphore subclass of NaClDesc.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_SEMAPHORE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_SEMAPHORE_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"

#include "native_client/src/shared/platform/nacl_semaphore.h"

EXTERN_C_BEGIN

struct NaClDescEffector;
struct NaClDescXferState;

struct NaClDescSemaphore {
  struct NaClDesc      base NACL_IS_REFCOUNT_SUBCLASS;
  struct NaClSemaphore sem;
};

int NaClDescSemaphoreCtor(struct NaClDescSemaphore  *self, int value)
    NACL_WUR;

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_SEMAPHORE_H_
