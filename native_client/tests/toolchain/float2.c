/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * More basic tests for floating point operations
 */

#include <stdio.h>
#include "native_client/tests/toolchain/utils.h"

/* NOTE: we use our own definitions instead of stdint.h's */
/* to ensure compatibility with printf format strings. */
/* Getting this right without casting across all our TCs */
/* is very hard otherwise. */
typedef signed int T_int32;
typedef unsigned int T_uint32;
typedef signed long long T_int64;
typedef unsigned long long T_uint64;

volatile T_int32 i32[] = {5, -5,};
volatile T_int64 i64[] = {5, -5, 1LL << 33, -1 * (1LL << 33)};

volatile T_uint32 u32[] = {5, 1<<31};
volatile T_uint64 u64[] = {5, 1LL<<63};

volatile float f32[] = { 5.6, 5.5, 5.4,
                         -5.6, -5.5, -5.4,
/* TODO(robertm): make this work across all compilers */
/*                          1e50, 1e10 */
/* http://code.google.com/p/nativeclient/issues/detail?id=2040 */
};

volatile double f64[] = { 5.6, 5.5, 5.4,
                          -5.6, -5.5, -5.4,
/* TODO(robertm): make this work across all compilers */
/*                         1e50, 1e10 */
};


int main(int argc, char* argv[]) {
  int i;

  for (i = 0; i < ARRAY_SIZE_UNSAFE(i32); ++i) {
    printf("i32: %d\n", i32[i]);
    printf("double %.6f\n", (double)i32[i]);
    printf("float  %.6f\n", (float)i32[i]);
    printf("\n");
  }

  for (i = 0; i < ARRAY_SIZE_UNSAFE(i64); ++i) {
    printf("i64: %lld\n", i64[i]);
    printf("double %.6f\n", (double)i64[i]);
    printf("float  %.6f\n", (float)i64[i]);
    printf("\n");
  }

  for (i = 0; i < ARRAY_SIZE_UNSAFE(u32); ++i) {
    printf("u32: %u\n", u32[i]);
    printf("double %.6f\n", (double)u32[i]);
    printf("float  %.6f\n", (float)u32[i]);
    printf("\n");
  }

  for (i = 0; i < ARRAY_SIZE_UNSAFE(u64); ++i) {
    printf("u64: %llu\n", u64[i]);
    printf("double %.6f\n", (double)u64[i]);
    printf("float  %.6f\n", (float)u64[i]);
    printf("\n");
  }

  for (i = 0; i < ARRAY_SIZE_UNSAFE(f32); ++i) {
    printf("f32:   %e\n", f32[i]);
    printf("int32_t  %d\n", (T_int32)f32[i]);
    printf("int64_t  %lld\n", (T_int64)f32[i]);
    if (f32[i] >= 0.0) {
      printf("uint32_t %u\n", (T_uint32)f32[i]);
      printf("uint64_t %llu\n", (T_uint64)f32[i]);
    }
    printf("\n");
  }

  for (i = 0; i < ARRAY_SIZE_UNSAFE(f64); ++i) {
    printf("f64:   %e\n", f64[i]);
    printf("int32_t  %d\n", (T_int32)f64[i]);
    printf("int64_t  %lld\n", (T_int64)f64[i]);
    if (f64[i] >= 0.0) {
      printf("uint32_t %u\n", (T_uint32)f64[i]);
      printf("uint64_t %llu\n", (T_uint64)f64[i]);
    }
    printf("\n");
  }

  return 0;
}
