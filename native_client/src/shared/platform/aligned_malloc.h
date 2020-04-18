/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_PLATFORM_ALIGNED_ALLOC_H_
#define NATIVE_CLIENT_SRC_SHARED_PLATFORM_ALIGNED_ALLOC_H_ 1

#include <stdlib.h>

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Allocates a block of memory with the given alignment, which must be
 * a power of 2 and a multiple of sizeof(void *).  Returns NULL on
 * failure.
 */
void *NaClAlignedMalloc(size_t size, size_t alignment);

/*
 * Frees a block of memory that was returned by NaClAlignedMalloc().
 */
void NaClAlignedFree(void *block);

EXTERN_C_END

#endif
