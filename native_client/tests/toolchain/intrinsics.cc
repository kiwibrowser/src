/*
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

// use volatile to confuse llmv optimizer
volatile long long s64a = 0x12345678;
volatile long long s64b = 0x1;

volatile unsigned long long u64a = 0x12345678;
volatile unsigned long long u64b = 0x1;

volatile int s32a = 0x1234;
volatile int s32b = 0x1;

volatile unsigned int u32a = 0x1234;
volatile unsigned int u32b = 0x1;

volatile int failures = 0;

template<typename Integer> void Check(const char* mode, Integer a, Integer b) {
  for (int i = 0; i < 16; ++i) {
    Integer c = a * b;
    // this relies on b == 1
    if (c >> i != a) {
      ++failures;
      printf("failure in mult %s %d\n", mode, i);
    }

    c += failures; /* hopefully a nop */
    Integer div = c / (b + 1);
    b += failures; /* hopefully a nop */
    Integer rem = c % (b + 1);
    b += failures; /* hopefully a nop */
    if (div * (b + 1) + rem != c) {
      ++failures;
      printf("failure in div/mod %s %d\n", mode, i);
    }

    b <<= 1;
  }
}

int main() {
  Check<int>("s32", s32a, s32b);
  Check<unsigned int>("u32", u32a, u32b);
  Check<long long>("s64", s64a, s64b);
  Check<unsigned long long>("u64", u64a, u64b);
  return 55 + failures;
}
