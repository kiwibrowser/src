/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime.  Transferrable Mach-based shared memory objects.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_OSX_NACL_DESC_IMC_SHM_MACH_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_OSX_NACL_DESC_IMC_SHM_MACH_H_

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_base.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"

/* get nacl_off64_t */
#include "native_client/src/shared/platform/nacl_host_desc.h"

EXTERN_C_BEGIN

struct NaClDesc;

struct NaClDescImcShmMach {
  struct NaClDesc base NACL_IS_REFCOUNT_SUBCLASS;
  mach_port_t h;
  nacl_off64_t size;
  /* note nacl_off64_t so struct stat incompatible */
};

/*
 * Constructor: initialize the NaClDescImcShmMach object with the given size.
 */
int NaClDescImcShmMachAllocCtor(struct NaClDescImcShmMach *self,
                                nacl_off64_t size, int executable) NACL_WUR;

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_OSX_NACL_DESC_IMC_SHM_MACH_H_
