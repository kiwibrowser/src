/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime.  NaClDescInvalid subclass of NaClDesc.
 * This is a singleton subclass.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_INVALID_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_INVALID_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"

EXTERN_C_BEGIN

/*
 * Invalid descriptor
 * This class is a singleton used to enable passing a designated "invalid"
 * descriptor via RPCs for error conditions.
 */
struct NaClDescInvalid {
  struct NaClDesc base NACL_IS_REFCOUNT_SUBCLASS;
};

int NaClDescInvalidInternalize(struct NaClDesc               **baseptr,
                               struct NaClDescXferState      *xfer)
    NACL_WUR;

/* Initialize and tear down the state for maintaining the singleton. */
void NaClDescInvalidInit(void);
void NaClDescInvalidFini(void);

/* A factory method for returning the singleton. */
struct NaClDescInvalid const *NaClDescInvalidMake(void);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_INVALID_H_ */
