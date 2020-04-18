/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* types_memory_model.h - Model memory addresses and memory size.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_x86_TYPES_MEMORY_MODEL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_x86_TYPES_MEMORY_MODEL_H_

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

/* Define the width of an address based on the wordsize.
 * NaClPcAddress - An address into memory.
 * NaClPcNumber - Number to allow the computation of relative address offsets
 *            (both positive and negative).
 * NaClMemorySize - The number of bytes in memory.
 */
#if NACL_BUILD_SUBARCH == 64
typedef uint64_t NaClPcAddress;
#define NACL_PRIxNaClPcAddress    NACL_PRIx64
#define NACL_PRIxNaClPcAddressAll "016" NACL_PRIx64

typedef int64_t NaClPcNumber;
#define NACL_PRIdNaClPcNumber NACL_PRId64

typedef uint64_t NaClMemorySize;
#define NACL_PRIxNaClMemorySize NACL_PRIx64
#define NACL_PRIxNaClMemorySizeAll "016" NACL_PRIx64
#else
typedef uint32_t NaClPcAddress;
#define NACL_PRIxNaClPcAddress     NACL_PRIx32
#define NACL_PRIxNaClPcAddressAll "08" NACL_PRIx32

typedef int32_t NaClPcNumber;
#define NACL_PRIdNaClPcNumber NACL_PRId32

typedef uint32_t NaClMemorySize;
#define NACL_PRIxNaClMemorySize NACL_PRIx32
#define NACL_PRIxNaClMemorySizeAll "08" NACL_PRIx32
#endif

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_x86_TYPES_MEMORY_MODEL_H_ */
