/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST(SZ, FMT, ...) do {                                 \
    char buf[SZ + 1];                                           \
    int res;                                                    \
    /* Make sure null-termination occurs. */                    \
    memset(buf, '@', SZ);                                       \
    printf("snprintf(buf, " #SZ ", \"%s\", ...) => ", FMT);     \
    /* Pass NULL as a buffer if SZ is 0, which is allowed. */   \
    res = snprintf(SZ ? buf : NULL, SZ, FMT, __VA_ARGS__);      \
    printf("%i ", res);                                         \
    if (res < 0) {                                              \
      printf("FAILED\n");                                       \
    }                                                           \
    else if (res > SZ) {                                        \
      printf("truncated: \"%s\"\n", SZ ? buf : "");             \
    }                                                           \
    else if (res == SZ) {                                       \
      printf("truncated for null: \"%s\"\n", buf);              \
    }                                                           \
    else {                                                      \
      printf("\"%s\"\n", buf);                                  \
    }                                                           \
  } while (0)

int main(void) {
  TEST( 0, "%s", "Small");
  TEST(14, "%s, %s!", "Hello", "World");
  TEST(13, "%s, %s!", "Hello", "World");
  TEST(12, "%s, %s!", "Hello", "World");
  TEST(12, "Answer = %i", 42);
  TEST(11, "Answer = %i", 42);
  TEST(10, "Answer = %i", 42);
  TEST(19, "Address = %x", 0xdeadbeef);
  TEST(18, "Address = %x", 0xdeadbeef);
  TEST(17, "Address = %x", 0xdeadbeef);
  TEST(14, "PI = %f", M_PI);
  TEST(13, "PI = %f", M_PI);
  TEST(12, "PI = %f", M_PI);

  return 0;
}
