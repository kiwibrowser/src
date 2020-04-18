/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"

int main(void) {
  int num_errors = 0;

#define TEST(T, M, V, fmt) do {                                \
    printf("%s: expect %" fmt "\n", #T, (T) V);                \
    if (M(T) != V) {                                           \
      printf("Error: " #M " is %" fmt ", expected %" fmt "\n", \
             M(T), (T) V);                                     \
      ++num_errors;                                            \
    } else {                                                   \
      printf("OK\n");                                          \
    }                                                          \
  } while (0)

  TEST(uint8_t, NACL_UMAX_VAL, 255, NACL_PRIu8);
  TEST(int8_t, NACL_MAX_VAL, 127, NACL_PRId8);
  TEST(uint16_t, NACL_UMAX_VAL, 65535, NACL_PRIu16);
  TEST(int16_t, NACL_MAX_VAL, 32767, NACL_PRId16);
  TEST(uint32_t, NACL_UMAX_VAL, 4294967295U, NACL_PRIu32);
  TEST(int32_t, NACL_MAX_VAL,  2147483647, NACL_PRId32);
  TEST(uint64_t, NACL_UMAX_VAL, (uint64_t) 18446744073709551615ULL,
       NACL_PRIu64);
  TEST(int64_t, NACL_MAX_VAL, (int64_t) 9223372036854775807LL,
       NACL_PRId64);

  TEST(uint8_t, NACL_UMIN_VAL, 0, NACL_PRIu8);
  TEST(int8_t, NACL_MIN_VAL, -128, NACL_PRId8);
  TEST(uint16_t, NACL_UMIN_VAL, 0, NACL_PRIu16);
  TEST(int16_t, NACL_MIN_VAL, -32768, NACL_PRId16);
  TEST(uint32_t, NACL_UMIN_VAL, 0, NACL_PRIu32);
  TEST(int32_t, NACL_MIN_VAL,  (-1 + (int32_t) -2147483647), NACL_PRId32);
  TEST(uint64_t, NACL_UMIN_VAL, (uint64_t) 0, NACL_PRIu64);
  TEST(int64_t, NACL_MIN_VAL, (-1 + (int64_t) -9223372036854775807LL),
       NACL_PRId64);
#undef TEST

  /* port_win and/or local system defns */
#define TEST(T, MAX_VAL_MACRO,EXPECTED_VALUE, FMT) do {  \
    printf("expected %" FMT ", actual %" FMT "\n",    \
           (T) EXPECTED_VALUE, (T) MAX_VAL_MACRO);    \
    if (EXPECTED_VALUE != MAX_VAL_MACRO) {            \
      ++num_errors;                                   \
      printf("Error\n");                              \
    } else {                                          \
      printf("OK\n");                                 \
    }                                                 \
  } while (0)

  TEST(uint8_t, UINT8_MAX, 255, NACL_PRIu8);

  TEST(int8_t, INT8_MAX, 127, NACL_PRId8);
  TEST(int8_t, INT8_MIN, -128, NACL_PRId8);

  TEST(uint16_t, UINT16_MAX, 65535, NACL_PRIu16);

  TEST(int16_t, INT16_MAX, 32767, NACL_PRId16);
  TEST(int16_t, INT16_MIN, -32768, NACL_PRId16);

  TEST(uint32_t, UINT32_MAX, 4294967295U, NACL_PRIu32);

  TEST(int32_t, INT32_MAX, 2147483647, NACL_PRId32);
  TEST(int32_t, INT32_MIN, -2147483647-1, NACL_PRId32);

  TEST(uint64_t, UINT64_MAX, (uint64_t) 18446744073709551615ULL, NACL_PRIu64);

  TEST(int64_t, INT64_MAX, (int64_t) 9223372036854775807LL, NACL_PRId64);
  TEST(int64_t, INT64_MIN, (int64_t) -9223372036854775807LL-1, NACL_PRId64);

  if (0 != num_errors) {
    printf("FAILED\n");
  } else {
    printf("PASSED\n");
  }
  return num_errors;
}
