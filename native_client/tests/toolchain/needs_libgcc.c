/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc < 3)
    return 1;
  uint64_t a = atoll(argv[1]);
  uint64_t b = atoll(argv[2]);
  /* 64-bit divide uses a libgcc/compiler-rt call on x86-32 and ARM. */
  if (a / b == 3ULL)
    return 0;
  return 1;
}
