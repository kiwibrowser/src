/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * A NaClDesc subclass that exposes a /dev/null interface.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_NULL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_NULL_H_

#include "native_client/src/trusted/desc/nacl_desc_base.h"

EXTERN_C_BEGIN

struct NaClDescNull {
  struct NaClDesc base NACL_IS_REFCOUNT_SUBCLASS;
};

int NaClDescNullCtor(struct NaClDescNull *self);

int NaClDescNullInternalize(struct NaClDesc **out_desc,
                            struct NaClDescXferState *xfer)
    NACL_WUR;

EXTERN_C_END

#endif
