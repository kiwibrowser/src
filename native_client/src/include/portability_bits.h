/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_BITS_H_
#define NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_BITS_H_ 1

/*
 * Portable bit functions.
 *
 * Not all platforms offer fast intrinsics for these functions, and some
 * compilers require checking CPUID at runtime before using the intrinsic.
 *
 * We instead use portable and reasonably-fast implementations, while
 * avoiding implementations with large lookup tables.
 *
 * When adding functions, also add tests in:
 *   tests/unittests/trusted/bits/bits_test.cc
 */

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/nacl_macros.h"

namespace nacl {

// Only the specialized templates should be instantiated, getting
// a linker error with these functions means an unsupported type was used.

template<typename T> INLINE int PopCount(T /* v */);
template<typename T> INLINE T BitReverse(T /* v */);
template<typename T> INLINE int CountTrailingZeroes(T /* v */);
template<typename T> INLINE int CountLeadingZeroes(T /* v */);

// Implementations for the above templates.

template<> INLINE int PopCount<uint8_t>(uint8_t v) {
  // Small table lookup.
  static const uint8_t tbl[32] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5
  };
  return tbl[v & 0xf] + tbl[v >> 4];
}
template<> INLINE int PopCount<uint16_t>(uint16_t v) {
  return PopCount<uint8_t>(v & 0xff) + PopCount<uint8_t>(v >> 8);
}
template<> INLINE int PopCount<uint32_t>(uint32_t v) {
  // See Stanford bithacks, counting bits set in parallel, "best method":
  // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
  v = v - ((v >> 1) & 0x55555555);
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
  return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}
template<> INLINE int PopCount<uint64_t>(uint64_t v) {
  return PopCount<uint32_t>((uint32_t)v) + PopCount<uint32_t>(v >> 32);
}

template<> INLINE uint32_t BitReverse<uint32_t>(uint32_t v) {
  // See Hacker's Delight, first edition, figure 7-1.
  v = ((v & 0x55555555) << 1) | ((v >> 1) & 0x55555555);
  v = ((v & 0x33333333) << 2) | ((v >> 2) & 0x33333333);
  v = ((v & 0x0F0F0F0F) << 4) | ((v >> 4) & 0x0F0F0F0F);
  v = (v << 24) | ((v & 0xFF00) << 8) |
      ((v >> 8) & 0xFF00) | (v >> 24);
  return v;
}

template<> INLINE int CountTrailingZeroes<uint32_t>(uint32_t v) {
  // See Stanford bithacks, count the consecutive zero bits (trailing) on the
  // right with multiply and lookup:
  // http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
  static const uint8_t tbl[32] = {
    0,   1, 28,  2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17,  4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18,  6, 11,  5, 10, 9
  };
  return v ?
      (int)tbl[((uint32_t)((v & -(int32_t)v) * 0x077CB531U)) >> 27] :
      -1;
}

template<> INLINE int CountLeadingZeroes<uint32_t>(uint32_t v) {
  // See Stanford bithacks, find the log base 2 of an N-bit integer in
  // O(lg(N)) operations with multiply and lookup:
  // http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
  static const uint8_t tbl[32] = {
    31, 22, 30, 21, 18, 10, 29,  2, 20, 17, 15, 13, 9,  6, 28, 1,
    23, 19, 11,  3, 16, 14,  7, 24, 12,  4,  8, 25, 5, 26, 27, 0
  };
  v = v | (v >>  1);
  v = v | (v >>  2);
  v = v | (v >>  4);
  v = v | (v >>  8);
  v = v | (v >> 16);
  return v ?
      (int)tbl[((uint32_t)(v * 0x07C4ACDDU)) >> 27] :
      -1;
}

}  // namespace nacl

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_BITS_H_ */
