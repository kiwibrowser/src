/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* @file
 *
 * Implementation of effector subclass used only for service runtime's
 * gio_shm object for mapping/unmapping shared memory in *trusted*
 * address space.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_EFFECTOR_TRUSTED_MEM_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_EFFECTOR_TRUSTED_MEM_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/desc/nacl_desc_effector.h"

EXTERN_C_BEGIN

extern const struct NaClDescEffector NaClDescEffectorTrustedMemStruct;

static INLINE struct NaClDescEffector *NaClDescEffectorTrustedMem(void) {
  /* This struct is read-only, although other NaClDescEffectors need not be. */
  return (struct NaClDescEffector *) &NaClDescEffectorTrustedMemStruct;
}

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_EFFECTOR_TRUSTED_MEM_H_
