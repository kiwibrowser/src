/*
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl test for super simple program not using newlib
 */

#include "native_client/tests/barebones/barebones.h"

volatile unsigned stuff[100];

int main(void) {
  stuff[0] = stuff[1];
  NACL_SYSCALL(exit)(55);
  /* UNREACHABLE */
  return 0;
}
