/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl test for super simple program not using newlib
 */

#include "barebones.h"

/* fib(9) == 55 */
/* NOTE: must not be const to prevent llvm optimizations */
int startval = 9;


static int fib(int val) {
  if (val <= 1) {
    return 1;
  } else {
    return fib(val - 1) + fib(val - 2);
  }
}


int main(void) {
  NACL_SYSCALL(exit)(fib(startval));
  /* UNREACHABLE */
  return 0;
}
