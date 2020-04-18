/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file contains bitmap array manipulation functions.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_BITMAP_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_BITMAP_H_

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"

#if NACL_WINDOWS
# define FORCEINLINE __forceinline
#else
# define FORCEINLINE __inline __attribute__ ((always_inline))
#endif

typedef NACL_CONCAT(NACL_CONCAT(uint, NACL_HOST_WORDSIZE), _t) bitmap_word;

static INLINE bitmap_word *BitmapAllocate(size_t indexes) {
  size_t word_count = ((indexes + NACL_HOST_WORDSIZE - 1) / NACL_HOST_WORDSIZE);
  NACL_COMPILE_TIME_ASSERT((NACL_HOST_WORDSIZE / 8) == sizeof(bitmap_word));
  return calloc(word_count, sizeof(bitmap_word));
}

static FORCEINLINE int BitmapIsBitSet(bitmap_word *bitmap, size_t index) {
  return (bitmap[index / NACL_HOST_WORDSIZE] &
                       (((bitmap_word)1) << (index % NACL_HOST_WORDSIZE))) != 0;
}

static FORCEINLINE void BitmapSetBit(bitmap_word *bitmap, size_t index) {
  bitmap[index / NACL_HOST_WORDSIZE] |=
                               ((bitmap_word)1) << (index % NACL_HOST_WORDSIZE);
}

static FORCEINLINE void BitmapClearBit(bitmap_word *bitmap, size_t index) {
  bitmap[index / NACL_HOST_WORDSIZE] &=
                            ~(((bitmap_word)1) << (index % NACL_HOST_WORDSIZE));
}

/* All the bits must be in a single 32-bit bundle.  */
static FORCEINLINE int BitmapIsAnyBitSet(bitmap_word *bitmap,
                                         size_t index, size_t bits) {
  return (bitmap[index / NACL_HOST_WORDSIZE] &
       (((((bitmap_word)1) << bits) - 1) << (index % NACL_HOST_WORDSIZE))) != 0;
}

/* All the bits must be in a single 32-bit bundle. */
static FORCEINLINE void BitmapSetBits(bitmap_word *bitmap,
                                      size_t index,
                                      size_t bits) {
  bitmap[index / NACL_HOST_WORDSIZE] |=
               ((((bitmap_word)1) << bits) - 1) << (index % NACL_HOST_WORDSIZE);
}

/* All the bits must be in a single 32-bit bundle. */
static FORCEINLINE void BitmapClearBits(bitmap_word *bitmap,
                                        size_t index, size_t bits) {
  bitmap[index / NACL_HOST_WORDSIZE] &=
            ~(((((bitmap_word)1) << bits) - 1) << (index % NACL_HOST_WORDSIZE));
}

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_VALIDATOR_INTERNAL_H_ */
