/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This header file declares INTERNAL USE ONLY interfaces.  These
 * interfaces are used for testing, and this file should never be
 * included by relying code.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_VALIDATION_CACHE_INTERNAL_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_VALIDATION_CACHE_INTERNAL_H__

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

struct NaClRichFileInfo;

int NaClSerializeNaClDescMetadata(
    const struct NaClRichFileInfo *info,
    uint8_t **buffer,
    uint32_t *buffer_length);

int NaClDeserializeNaClDescMetadata(
    const uint8_t *buffer,
    uint32_t buffer_length,
    struct NaClRichFileInfo *info);

EXTERN_C_END

#endif

