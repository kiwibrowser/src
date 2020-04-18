/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl test for super simple program not using newlib
 */

#include "barebones.h"

/* NOTE: must not be const to prevent llvm optimizations */
int startval = 20;

static int switchme(int val) {
  switch(val) {
   case 1:
    return val * val;
   case 2:
    myprint("hi");
    return 0;
   case 3:
    return startval + 100;
   case 4:
    return 5;
   case 5:
    myprint("hi there");
    return 66;
   case 20:
    return 55;
   default:
    myprint("error");
    return 66;
  }
}

int main(void) {
  NACL_SYSCALL(exit)(switchme(startval));
  /* UNREACHABLE */
  return 0;
}
