/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Internal interfaces.  Not for public consumption.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NRD_XFER_INTERN_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NRD_XFER_INTERN_H_

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"

EXTERN_C_BEGIN

void NaClNrdXferIncrTagOverhead(size_t *byte_count,
                                size_t *handle_count);

enum NaClDescTypeTag NaClNrdXferReadTypeTag(struct NaClDescXferState *xferp);

void NaClNrdXferWriteTypeTag(struct NaClDescXferState *xferp,
                             struct NaClDesc          *descp);

int NaClDescInternalizeFromXferBuffer(
    struct NaClDesc               **out_desc,
    struct NaClDescXferState      *xferp)
    NACL_WUR;

int NaClDescExternalizeToXferBuffer(struct NaClDescXferState  *xferp,
                                    struct NaClDesc           *out)
    NACL_WUR;

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_DESC_NRD_XFER_INTERN_H_ */
