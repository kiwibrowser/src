/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl test for super simple program not using newlib
 */

#include "barebones.h"


int main(void) {
  NACL_SYSCALL(exit)(55);
  /* UNREACHABLE */
  return 0;
}
