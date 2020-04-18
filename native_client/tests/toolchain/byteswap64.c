/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <endian.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

int main(void) {
  const uint64_t one = 1;
  const uint64_t swapped = htobe64(one);
  const uint64_t backswapped = be64toh(swapped);
  const bool equals = backswapped == one;
  printf("%#" PRIx64 " %c= %#" PRIx64 "\n",
         backswapped, equals ? '=' : '!', one);
  return equals ? 0 : 1;
}
