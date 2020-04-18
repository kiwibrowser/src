/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_VALIDATION_CACHE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_VALIDATION_CACHE_H_

#include <stddef.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/public/validation_cache.h"

EXTERN_C_BEGIN

extern int NaClCachingIsInexpensive(struct NaClValidationCache *cache,
                                    const struct
                                    NaClValidationMetadata *metadata);

/* Helper function for identifying the code being validated. */
extern void NaClAddCodeIdentity(uint8_t *data,
                                size_t size,
                                const struct NaClValidationMetadata *metadata,
                                struct NaClValidationCache *cache,
                                void *query);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_VALIDATION_CACHE_H_ */
