/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime.  Transferrable shared memory objects.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IMC_SHM_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IMC_SHM_H_

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/public/nacl_desc.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"

/*
 * get NaClHandle, which is a typedef and not a struct pointer, so
 * impossible to just forward declare.
 */
#include "native_client/src/shared/imc/nacl_imc_c.h"

/* get nacl_off64_t */
#include "native_client/src/shared/platform/nacl_host_desc.h"

EXTERN_C_BEGIN

struct NaClDesc;
struct NaClDescEffector;
struct NaClDescImcShm;
struct NaClDescXferState;
struct NaClHostDesc;
struct NaClMessageHeader;
struct nacl_abi_stat;

struct NaClDescImcShm {
  struct NaClDesc           base NACL_IS_REFCOUNT_SUBCLASS;
  NaClHandle                h;
  nacl_off64_t              size;
  /* note nacl_off64_t so struct stat incompatible */
};

int NaClDescImcShmInternalize(struct NaClDesc               **baseptr,
                              struct NaClDescXferState      *xfer)
    NACL_WUR;

/*
 * Constructor: initialize the NaClDescImcShm object based on the
 * underlying low-level IMC handle h which has the given size.
 */
int NaClDescImcShmCtor(struct NaClDescImcShm  *self,
                       NaClHandle             h,
                       nacl_off64_t           size)
    NACL_WUR;

/*
 * A convenience wrapper for above, where the shm object of a given
 * size is allocated first.
 */
int NaClDescImcShmAllocCtor(struct NaClDescImcShm  *self,
                            nacl_off64_t           size,
                            int                    executable)
    NACL_WUR;

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IMC_SHM_H_
