/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>

static int test_lahf_sahf(void) {
  /*
   * We aren't really trying to test sahf/lahf functionality
   * in detail, just that it is able to execute within the validator.
   */
  unsigned int test = 0;
  __asm__ __volatile__(
      "xor %%rax, %%rax\n\t" /* (%rax = 0x0, flags=0x46) */
      "sahf\n\t"  /* store ah (0) into flags (flags=0x2) (bit 1 is readonly) */
      "lahf"  /* load flags into %ah (%ah=0x2/%rax=0x200) */
      : "=a" (test));
  return test >> 8;
}


int main(void) {
  assert(test_lahf_sahf() == 2);
  return 0;
}

