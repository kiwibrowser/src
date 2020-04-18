/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Test that the GCC/LLVM vector extensions can be used from C code.
 * http://gcc.gnu.org/onlinedocs/gcc/Vector-Extensions.html
 * http://clang.llvm.org/docs/LanguageExtensions.html
 *
 * This test is thorough feature-wise, but not thorough in testing the
 * corner-case values. It merely ensures that the compiler can generate
 * code for vector extensions without rejecting it (e.g. with some
 * internal failure or validation error), and that the output it
 * generates is as specified by the extensions' specification (for the
 * few inputs it exercises). The test tries to exercise all vector types
 * and operations that are supported by PNaCl, and verifies that test
 * values generate the right result by comparing to a golden output
 * file. It does not test all the MIN/MAX values, nor does it test
 * undefined behavior: these are left to more thorough tests in this
 * directory.
 */

#include "native_client/src/include/nacl_macros.h"

#include <stdint.h>
#include <stdio.h>

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

/*
 *
 * The GCC/LLVM vector extensions represent the results of comparisons
 * as a vector of all-ones or all-zeros with the same vector bit width
 * and number of elements. They must be treated differently than their
 * corresponding type because floating-point values change their bit
 * representation through assignments when they hold NaN values.
 */
typedef int8_t I8_BOOL;
typedef int8_t U8_BOOL;
typedef int16_t I16_BOOL;
typedef int16_t U16_BOOL;
typedef int32_t I32_BOOL;
typedef int32_t U32_BOOL;
typedef int32_t F32_BOOL;

#define I8_FMT "i"
#define U8_FMT "u"
#define I16_FMT "i"
#define U16_FMT "u"
#define I32_FMT "i"
#define U32_FMT "u"
#define F32_FMT "f"

/* All elements in a boolean vector should print as 0 or -1. */
#define I8_BOOL_FMT "i"
#define U8_BOOL_FMT "i"
#define I16_BOOL_FMT "i"
#define U16_BOOL_FMT "i"
#define I32_BOOL_FMT "i"
#define U32_BOOL_FMT "i"
#define F32_BOOL_FMT "i"

/* All supported vector types are currently 128-bit wide. */
#define VEC_BYTES 16

/* Vector types corresponding to each supported basic types. */
typedef I8 VI8 __attribute__((vector_size(VEC_BYTES)));
typedef U8 VU8 __attribute__((vector_size(VEC_BYTES)));
typedef I16 VI16 __attribute__((vector_size(VEC_BYTES)));
typedef U16 VU16 __attribute__((vector_size(VEC_BYTES)));
typedef I32 VI32 __attribute__((vector_size(VEC_BYTES)));
typedef U32 VU32 __attribute__((vector_size(VEC_BYTES)));
typedef F32 VF32 __attribute__((vector_size(VEC_BYTES)));

/* Boolean vector types generate by comparisons on each vector type. */
typedef I8 VI8_BOOL __attribute__((vector_size(VEC_BYTES)));
typedef I8 VU8_BOOL __attribute__((vector_size(VEC_BYTES)));
typedef I16 VI16_BOOL __attribute__((vector_size(VEC_BYTES)));
typedef I16 VU16_BOOL __attribute__((vector_size(VEC_BYTES)));
typedef I32 VI32_BOOL __attribute__((vector_size(VEC_BYTES)));
typedef I32 VU32_BOOL __attribute__((vector_size(VEC_BYTES)));
typedef I32 VF32_BOOL __attribute__((vector_size(VEC_BYTES)));

#define PRINT(TYPE, VEC)                                                \
  do {                                                                  \
    NACL_COMPILE_TIME_ASSERT(sizeof(V##TYPE) ==                         \
                             VEC_BYTES); /* Vector must be 128 bits. */ \
    NACL_COMPILE_TIME_ASSERT(sizeof(TYPE) ==                            \
                             sizeof(VEC[0])); /* Type must match. */    \
    printf("{");                                                        \
    for (size_t i = 0; i != sizeof(V##TYPE) / sizeof(VEC[0]); ++i)      \
      printf("%" TYPE##_FMT ",", VEC[i]);                               \
    printf("}");                                                        \
  } while (0)

#define TEST_BINARY(TYPE, LHS, OP, RHS)                               \
  do {                                                                \
    NACL_COMPILE_TIME_ASSERT(sizeof(TYPE) ==                          \
                             sizeof(LHS[0])); /* Types must match. */ \
    NACL_COMPILE_TIME_ASSERT(sizeof(TYPE) ==                          \
                             sizeof(RHS[0])); /* Types must match. */ \
    const V##TYPE result = LHS OP RHS;                                \
    printf(#TYPE " ");                                                \
    PRINT(TYPE, LHS);                                                 \
    printf(" %s ", #OP);                                              \
    PRINT(TYPE, RHS);                                                 \
    printf(" = ");                                                    \
    PRINT(TYPE, result);                                              \
    printf("\n");                                                     \
  } while (0)

#define TEST_BINARY_COMPARISON(TYPE, LHS, OP, RHS)                    \
  do {                                                                \
    NACL_COMPILE_TIME_ASSERT(sizeof(TYPE) ==                          \
                             sizeof(LHS[0])); /* Types must match. */ \
    NACL_COMPILE_TIME_ASSERT(sizeof(TYPE) ==                          \
                             sizeof(RHS[0])); /* Types must match. */ \
    const V##TYPE##_BOOL result = LHS OP RHS;                         \
    printf(#TYPE " ");                                                \
    PRINT(TYPE, LHS);                                                 \
    printf(" %s ", #OP);                                              \
    PRINT(TYPE, RHS);                                                 \
    printf(" = ");                                                    \
    PRINT(TYPE##_BOOL, result);                                       \
    printf("\n");                                                     \
  } while (0)

#define TEST_UNARY(TYPE, OP, VAL)                                     \
  do {                                                                \
    NACL_COMPILE_TIME_ASSERT(sizeof(TYPE) ==                          \
                             sizeof(VAL[0])); /* Types must match. */ \
    const V##TYPE result = OP VAL;                                    \
    printf(#TYPE " %s ", #OP);                                        \
    PRINT(TYPE, VAL);                                                 \
    printf(" = ");                                                    \
    PRINT(TYPE, result);                                              \
    printf("\n");                                                     \
  } while (0)

#define TEST_BINARY_FP(TYPE, LHS, RHS)          \
  do {                                          \
    TEST_BINARY(TYPE, LHS, +, RHS);             \
    TEST_BINARY(TYPE, LHS, -, RHS);             \
    TEST_BINARY(TYPE, LHS, *, RHS);             \
    TEST_BINARY(TYPE, LHS, /, RHS);             \
    TEST_BINARY_COMPARISON(TYPE, LHS, ==, RHS); \
    TEST_BINARY_COMPARISON(TYPE, LHS, !=, RHS); \
    TEST_BINARY_COMPARISON(TYPE, LHS, <, RHS);  \
    TEST_BINARY_COMPARISON(TYPE, LHS, >, RHS);  \
    TEST_BINARY_COMPARISON(TYPE, LHS, <=, RHS); \
    TEST_BINARY_COMPARISON(TYPE, LHS, >=, RHS); \
  } while (0)

#define TEST_BINARY_INT(TYPE, LHS, RHS) \
  do {                                  \
    TEST_BINARY_FP(TYPE, LHS, RHS);     \
    TEST_BINARY(TYPE, LHS, %, RHS);     \
    TEST_BINARY(TYPE, LHS, &, RHS);     \
    TEST_BINARY(TYPE, LHS, |, RHS);     \
    TEST_BINARY(TYPE, LHS, ^, RHS);     \
    TEST_BINARY(TYPE, LHS, <<, RHS);    \
    TEST_BINARY(TYPE, LHS, >>, RHS);    \
  } while (0)

/*
 * TODO(jfb) Pre/post ++/-- don't seem to be supported. Neither does !.
 */
#define TEST_UNARY_FP(TYPE, VAL) \
  do {                           \
    TEST_UNARY(TYPE, +, VAL);    \
    TEST_UNARY(TYPE, -, VAL);    \
  } while (0)

#define TEST_UNARY_INT(TYPE, VAL) \
  do {                            \
    TEST_UNARY_FP(TYPE, VAL);     \
    TEST_UNARY(TYPE, ~, VAL);     \
  } while (0)

#define TEST_CAST(TYPE_TO, TYPE_FROM, VAL)                            \
  do {                                                                \
    NACL_COMPILE_TIME_ASSERT(sizeof(TYPE_TO) ==                       \
                             sizeof(VAL[0])); /* Types must match. */ \
    NACL_COMPILE_TIME_ASSERT(sizeof(TYPE_FROM) ==                     \
                             sizeof(VAL[0])); /* Types must match. */ \
    const V##TYPE_TO result = (V##TYPE_TO)VAL;                        \
    printf(#TYPE_FROM " cast to (V%s) ", #TYPE_TO);                   \
    PRINT(TYPE_FROM, VAL);                                            \
    printf(" = ");                                                    \
    PRINT(TYPE_TO, result);                                           \
    printf("\n");                                                     \
  } while (0)

#define TEST_8BIT_CAST(TYPE_FROM, VAL) \
  do {                                 \
    TEST_CAST(I8, TYPE_FROM, VAL);     \
    TEST_CAST(U8, TYPE_FROM, VAL);     \
  } while (0)

#define TEST_16BIT_CAST(TYPE_FROM, VAL) \
  do {                                  \
    TEST_CAST(I16, TYPE_FROM, VAL);     \
    TEST_CAST(U16, TYPE_FROM, VAL);     \
  } while (0)

#define TEST_32BIT_CAST(TYPE_FROM, VAL) \
  do {                                  \
    TEST_CAST(I32, TYPE_FROM, VAL);     \
    TEST_CAST(U32, TYPE_FROM, VAL);     \
    TEST_CAST(F32, TYPE_FROM, VAL);     \
  } while (0)

#define TEST_CONVERTVECTOR(TYPE_TO, TYPE_FROM, VAL)                     \
  do {                                                                  \
    NACL_COMPILE_TIME_ASSERT(sizeof(TYPE_TO) ==                         \
                             sizeof(VAL[0])); /* Types must match. */   \
    NACL_COMPILE_TIME_ASSERT(sizeof(TYPE_FROM) ==                       \
                             sizeof(VAL[0])); /* Types must match. */   \
    const V##TYPE_TO result = __builtin_convertvector(VAL, V##TYPE_TO); \
    printf(#TYPE_FROM " convertvector to V%s ", #TYPE_TO);              \
    PRINT(TYPE_FROM, VAL);                                              \
    printf(" = ");                                                      \
    PRINT(TYPE_TO, result);                                             \
    printf("\n");                                                       \
  } while (0)

#define TEST_8BIT_CONVERTVECTOR(TYPE_FROM, VAL) \
  do {                                          \
    TEST_CONVERTVECTOR(I8, TYPE_FROM, VAL);     \
    TEST_CONVERTVECTOR(U8, TYPE_FROM, VAL);     \
  } while (0)

#define TEST_16BIT_CONVERTVECTOR(TYPE_FROM, VAL) \
  do {                                           \
    TEST_CONVERTVECTOR(I16, TYPE_FROM, VAL);     \
    TEST_CONVERTVECTOR(U16, TYPE_FROM, VAL);     \
  } while (0)

#define TEST_32BIT_CONVERTVECTOR(TYPE_FROM, VAL) \
  do {                                           \
    TEST_CONVERTVECTOR(I32, TYPE_FROM, VAL);     \
    TEST_CONVERTVECTOR(U32, TYPE_FROM, VAL);     \
    TEST_CONVERTVECTOR(F32, TYPE_FROM, VAL);     \
  } while (0)

/*
 * Vector values used in tests.
 *
 * Initialize everything in a non-inlined function to make sure that
 * nothing gets pre-computed.
 */
VI8 vi8[2];
VU8 vu8[2];
VI16 vi16[2];
VU16 vu16[2];
VI32 vi32[2];
VU32 vu32[2];
VF32 vf32[2];
__attribute__((noinline)) void init(void) {
  /*
   * TODO(jfb) Test undefined behavior: shift bit bitwidth or larger,
   *           and divide by zero.
   */
  vi8[0] = (VI8) {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  vi8[1] = (VI8) {2, 1, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1};

  vu8[0] = (VU8) {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  vu8[1] = (VU8) {2, 1, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1};

  vi16[0] = (VI16) {1, 2, 3, 4, 5, 6, 7, 8};
  vi16[1] = (VI16) {1, 15, 14, 13, 12, 11, 10, 9};

  vu16[0] = (VU16) {1, 2, 3, 4, 5, 6, 7, 8};
  vu16[1] = (VU16) {1, 15, 14, 13, 12, 11, 10, 9};

  vi32[0] = (VI32) {1, 2, 3, 4};
  vi32[1] = (VI32) {16, 15, 14, 13};

  vu32[0] = (VU32) {1, 2, 3, 4};
  vu32[1] = (VU32) {16, 15, 14, 13};

  vf32[0] = (VF32) {1, 2, 3, 4};
  vf32[1] = (VF32) {16, 15, 14, 13};
}

__attribute__((noinline)) void test(void) {
  TEST_BINARY_INT(I8, vi8[0], vi8[1]);
  TEST_BINARY_INT(U8, vu8[0], vu8[1]);
  TEST_BINARY_INT(I16, vi16[0], vi16[1]);
  TEST_BINARY_INT(U16, vu16[0], vu16[1]);
  TEST_BINARY_INT(I32, vi32[0], vi32[1]);
  TEST_BINARY_INT(U32, vu32[0], vu32[1]);
  TEST_BINARY_FP(F32, vf32[0], vf32[1]);

  TEST_UNARY_INT(I8, vi8[0]);
  TEST_UNARY_INT(U8, vu8[0]);
  TEST_UNARY_INT(I16, vi16[0]);
  TEST_UNARY_INT(U16, vu16[0]);
  TEST_UNARY_INT(I32, vi32[0]);
  TEST_UNARY_INT(U32, vu32[0]);
  TEST_UNARY_FP(F32, vf32[0]);

  TEST_8BIT_CAST(I8, vi8[0]);
  TEST_8BIT_CAST(U8, vu8[0]);
  TEST_16BIT_CAST(I16, vi16[0]);
  TEST_16BIT_CAST(U16, vu16[0]);
  TEST_32BIT_CAST(I32, vi32[0]);
  TEST_32BIT_CAST(U32, vu32[0]);
  TEST_32BIT_CAST(F32, vf32[0]);

  TEST_8BIT_CONVERTVECTOR(I8, vi8[0]);
  TEST_8BIT_CONVERTVECTOR(U8, vu8[0]);
  TEST_16BIT_CONVERTVECTOR(I16, vi16[0]);
  TEST_16BIT_CONVERTVECTOR(U16, vu16[0]);
  TEST_32BIT_CONVERTVECTOR(I32, vi32[0]);
  TEST_32BIT_CONVERTVECTOR(U32, vu32[0]);
  TEST_32BIT_CONVERTVECTOR(F32, vf32[0]);
}

int main(void) {
  init();
  test();

  return 0;
}
