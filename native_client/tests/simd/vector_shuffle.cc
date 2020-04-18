/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Test that the LLVM vector shuffle can be used from C code.
 * http://clang.llvm.org/docs/LanguageExtensions.html#builtin-shufflevector
 *
 * We support the LLVM syntax only, the GCC equivalent isn't supported
 * by LLVM:
 * http://gcc.gnu.org/onlinedocs/gcc/Vector-Extensions.html
 *
 * This test ensures that the compiler can generate code for vector
 * shuffles without rejecting it (e.g. with some internal failure or
 * validation error), and that the output it generates is as specified
 * by the shuffle builtin's specification. It is fairly exhaustive and
 * covers many shuffle patterns in an attempt to utilize shuffle
 * patterns that compilers may choose to lower to better instructions on
 * certain architectures:
 *
 *  - Only the left-hand-side source is used to form the destination.
 *  - Only the right-hand-side source is used to form the destination.
 *  - Each index in a source is shuffled to each index in the destination.
 *  - The destination is identical to one of the inputs.
 *  - The destination is a broadcast from a single element in a source.
 *  - The destination interleaves elements from each source.
 *
 * The test is performed for all vector sizes and element sizes
 * currently supported by PNaCl.
 */

#include "native_client/src/include/nacl_macros.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

//  Functions are non-inlined to make sure that nothing gets
//  pre-computed at compile time.
#define NO_INLINE __attribute__((noinline))

/*
 * Basic types that are supported inside vectors.
 *
 * TODO(jfb) Handle 64-bit int and double.
 */
typedef int8_t I8;
typedef uint8_t U8;
typedef int16_t I16;
typedef uint16_t U16;
typedef int32_t I32;
typedef uint32_t U32;
typedef float F32;

// All supported vector types are currently 128-bit wide.
#define VEC_BYTES 16

// Vector types corresponding to each supported basic type.
typedef I8 VI8 __attribute__((vector_size(VEC_BYTES)));
typedef U8 VU8 __attribute__((vector_size(VEC_BYTES)));
typedef I16 VI16 __attribute__((vector_size(VEC_BYTES)));
typedef U16 VU16 __attribute__((vector_size(VEC_BYTES)));
typedef I32 VI32 __attribute__((vector_size(VEC_BYTES)));
typedef U32 VU32 __attribute__((vector_size(VEC_BYTES)));
typedef F32 VF32 __attribute__((vector_size(VEC_BYTES)));

// Reformat some types so they print nicely. Leave others as-is.
int32_t format(int8_t v) { return v; }
uint32_t format(uint8_t v) { return v; }
int32_t format(int16_t v) { return v; }
uint32_t format(uint16_t v) { return v; }
template <typename T> T format(T v) { return v; }

template <typename T>
NO_INLINE std::string print(const T v) {
  std::stringstream ss;
  ss << '{';
  for (size_t i = 0, e = sizeof(v) / sizeof(v[0]); i != e; ++i)
    ss << format(v[i]) << (i != e - 1 ? ',' : '}');
  return ss.str();
}

#define TEST_SHUFFLE(TYPE, LHS, RHS, ...)                                  \
  do {                                                                     \
    NACL_COMPILE_TIME_ASSERT(sizeof(TYPE) ==                               \
                             sizeof(LHS[0])); /* Types must match. */      \
    NACL_COMPILE_TIME_ASSERT(sizeof(TYPE) ==                               \
                             sizeof(RHS[0])); /* Types must match. */      \
    const V##TYPE result = __builtin_shufflevector(LHS, RHS, __VA_ARGS__); \
    std::cout << #TYPE " __builtin_shufflevector(" << print(LHS) << ", "   \
              << print(RHS) << ", " #__VA_ARGS__ ") = " << print(result)   \
              << std::endl;                                                \
  } while (0)

// Shuffle BASE into different locations, filling with FILL otherwise.
#define TEST_8BIT_SHUFFLE_STEP(T, L, R, BASE, FILL)                         \
  do {                                                                      \
    if (FILL != BASE) {                                                     \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL, \
                   FILL, FILL, FILL, FILL, FILL, FILL, FILL, BASE);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL, \
                   FILL, FILL, FILL, FILL, FILL, FILL, BASE, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL, \
                   FILL, FILL, FILL, FILL, FILL, BASE, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL, \
                   FILL, FILL, FILL, FILL, BASE, FILL, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL, \
                   FILL, FILL, FILL, BASE, FILL, FILL, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL, \
                   FILL, FILL, BASE, FILL, FILL, FILL, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL, \
                   FILL, BASE, FILL, FILL, FILL, FILL, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL, \
                   BASE, FILL, FILL, FILL, FILL, FILL, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, FILL, FILL, BASE, \
                   FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, FILL, BASE, FILL, \
                   FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, BASE, FILL, FILL, \
                   FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, BASE, FILL, FILL, FILL, \
                   FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, BASE, FILL, FILL, FILL, FILL, \
                   FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, FILL, BASE, FILL, FILL, FILL, FILL, FILL, \
                   FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, FILL, BASE, FILL, FILL, FILL, FILL, FILL, FILL, \
                   FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL);         \
      TEST_SHUFFLE(T, L, R, BASE, FILL, FILL, FILL, FILL, FILL, FILL, FILL, \
                   FILL, FILL, FILL, FILL, FILL, FILL, FILL, FILL);         \
    }                                                                       \
    /* Broadcast: */                                                        \
    TEST_SHUFFLE(T, L, R, BASE, BASE, BASE, BASE, BASE, BASE, BASE, BASE,   \
                 BASE, BASE, BASE, BASE, BASE, BASE, BASE, BASE);           \
  } while (0)

#define TEST_8BIT_SHUFFLE(T, L, R)                                            \
  do {                                                                        \
    std::cout << "Shuffle L only, ignore R:" << std::endl;                    \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 0, 0);                                    \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 1, 0);                                    \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 2, 0);                                    \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 3, 0);                                    \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 4, 0);                                    \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 5, 0);                                    \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 6, 0);                                    \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 7, 0);                                    \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 8, 0);                                    \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 9, 0);                                    \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 10, 0);                                   \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 11, 0);                                   \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 12, 0);                                   \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 13, 0);                                   \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 14, 0);                                   \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 15, 0);                                   \
    std::cout << "Shuffle R only, ignore L:" << std::endl;                    \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 16, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 17, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 18, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 19, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 20, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 21, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 22, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 23, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 24, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 25, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 26, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 27, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 28, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 29, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 30, 16);                                  \
    TEST_8BIT_SHUFFLE_STEP(T, L, R, 31, 16);                                  \
    std::cout << "Identity:" << std::endl;                                    \
    TEST_SHUFFLE(T, L, R, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,   \
                 15);                                                         \
    TEST_SHUFFLE(T, L, R, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, \
                 29, 30, 31);                                                 \
    std::cout << "Interleave:" << std::endl;                                  \
    TEST_SHUFFLE(T, L, R, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, \
                 23);                                                         \
    TEST_SHUFFLE(T, L, R, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14,   \
                 30, 15, 31);                                                 \
  } while (0)

// Shuffle BASE into different locations, filling with FILL otherwise.
#define TEST_16BIT_SHUFFLE_STEP(T, L, R, BASE, FILL)                         \
  do {                                                                       \
    if (FILL != BASE) {                                                      \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, FILL, FILL, BASE); \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, FILL, BASE, FILL); \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, FILL, BASE, FILL, FILL); \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, FILL, BASE, FILL, FILL, FILL); \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, BASE, FILL, FILL, FILL, FILL); \
      TEST_SHUFFLE(T, L, R, FILL, FILL, BASE, FILL, FILL, FILL, FILL, FILL); \
      TEST_SHUFFLE(T, L, R, FILL, BASE, FILL, FILL, FILL, FILL, FILL, FILL); \
      TEST_SHUFFLE(T, L, R, BASE, FILL, FILL, FILL, FILL, FILL, FILL, FILL); \
    }                                                                        \
    /* Broadcast: */                                                         \
    TEST_SHUFFLE(T, L, R, BASE, BASE, BASE, BASE, BASE, BASE, BASE, BASE);   \
  } while (0)

#define TEST_16BIT_SHUFFLE(T, L, R)                        \
  do {                                                     \
    std::cout << "Shuffle L only, ignore R:" << std::endl; \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 0, 0);                \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 1, 0);                \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 2, 0);                \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 3, 0);                \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 4, 0);                \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 5, 0);                \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 6, 0);                \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 7, 0);                \
    std::cout << "Shuffle R only, ignore L:" << std::endl; \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 8, 8);                \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 9, 8);                \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 10, 8);               \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 11, 8);               \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 12, 8);               \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 13, 8);               \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 14, 8);               \
    TEST_16BIT_SHUFFLE_STEP(T, L, R, 15, 8);               \
    std::cout << "Identity:" << std::endl;                 \
    TEST_SHUFFLE(T, L, R, 0, 1, 2, 3, 4, 5, 6, 7);         \
    TEST_SHUFFLE(T, L, R, 8, 9, 10, 11, 12, 13, 14, 15);   \
    std::cout << "Interleave:" << std::endl;               \
    TEST_SHUFFLE(T, L, R, 0, 8, 1, 9, 2, 10, 3, 11);       \
    TEST_SHUFFLE(T, L, R, 4, 12, 5, 13, 6, 14, 7, 15);     \
  } while (0)

// Shuffle BASE into different locations, filling with FILL otherwise.
#define TEST_32BIT_SHUFFLE_STEP(T, L, R, BASE, FILL) \
  do {                                               \
    if (FILL != BASE) {                              \
      TEST_SHUFFLE(T, L, R, FILL, FILL, FILL, BASE); \
      TEST_SHUFFLE(T, L, R, FILL, FILL, BASE, FILL); \
      TEST_SHUFFLE(T, L, R, FILL, BASE, FILL, FILL); \
      TEST_SHUFFLE(T, L, R, BASE, FILL, FILL, FILL); \
    }                                                \
    /* Broadcast: */                                 \
    TEST_SHUFFLE(T, L, R, BASE, BASE, BASE, BASE);   \
  } while (0)

#define TEST_32BIT_SHUFFLE(T, L, R)                        \
  do {                                                     \
    std::cout << "Shuffle L only, ignore R:" << std::endl; \
    TEST_32BIT_SHUFFLE_STEP(T, L, R, 0, 0);                \
    TEST_32BIT_SHUFFLE_STEP(T, L, R, 1, 0);                \
    TEST_32BIT_SHUFFLE_STEP(T, L, R, 2, 0);                \
    TEST_32BIT_SHUFFLE_STEP(T, L, R, 3, 0);                \
    std::cout << "Shuffle R only, ignore L:" << std::endl; \
    TEST_32BIT_SHUFFLE_STEP(T, L, R, 4, 4);                \
    TEST_32BIT_SHUFFLE_STEP(T, L, R, 5, 4);                \
    TEST_32BIT_SHUFFLE_STEP(T, L, R, 6, 4);                \
    TEST_32BIT_SHUFFLE_STEP(T, L, R, 7, 4);                \
    std::cout << "Identity:" << std::endl;                 \
    TEST_SHUFFLE(T, L, R, 0, 1, 2, 3);                     \
    TEST_SHUFFLE(T, L, R, 4, 5, 6, 7);                     \
    std::cout << "Interleave:" << std::endl;               \
    TEST_SHUFFLE(T, L, R, 0, 4, 1, 5);                     \
    TEST_SHUFFLE(T, L, R, 2, 6, 3, 7);                     \
  } while (0)

// Vector values used in tests.
VI8 vi8[2];
VU8 vu8[2];
VI16 vi16[2];
VU16 vu16[2];
VI32 vi32[2];
VU32 vu32[2];
VF32 vf32[2];
NO_INLINE void init(void) {
  vi8[0] = (VI8) {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  vi8[1] =
      (VI8) {17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};

  vu8[0] = (VU8) {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  vu8[1] =
      (VU8) {17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};

  vi16[0] = (VI16) {1, 2, 3, 4, 5, 6, 7, 8};
  vi16[1] = (VI16) {9, 10, 11, 12, 13, 14, 15, 16};

  vu16[0] = (VU16) {1, 2, 3, 4, 5, 6, 7, 8};
  vu16[1] = (VU16) {9, 10, 11, 12, 13, 14, 15, 16};

  vi32[0] = (VI32) {1, 2, 3, 4};
  vi32[1] = (VI32) {5, 6, 7, 8};

  vu32[0] = (VU32) {1, 2, 3, 4};
  vu32[1] = (VU32) {5, 6, 7, 8};

  vf32[0] = (VF32) {1, 2, 3, 4};
  vf32[1] = (VF32) {5, 6, 7, 8};
}

NO_INLINE void test(void) {
  std::cout << "8 bit shuffles:" << std::endl;
  TEST_8BIT_SHUFFLE(I8, vi8[0], vi8[1]);
  TEST_8BIT_SHUFFLE(U8, vu8[0], vu8[1]);
  std::cout << "16 bit shuffles:" << std::endl;
  TEST_16BIT_SHUFFLE(I16, vi16[0], vi16[1]);
  TEST_16BIT_SHUFFLE(U16, vu16[0], vu16[1]);
  std::cout << "32 bit shuffles:" << std::endl;
  TEST_32BIT_SHUFFLE(I32, vi32[0], vi32[1]);
  TEST_32BIT_SHUFFLE(U32, vu32[0], vu32[1]);
  TEST_32BIT_SHUFFLE(F32, vf32[0], vf32[1]);
}

int main(void) {
  init();
  test();

  return 0;
}
