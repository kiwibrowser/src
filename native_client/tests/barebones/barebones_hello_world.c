/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl test for simple hello world not using newlib
 */

#include "barebones.h"


int main(void) {
  myprint("@null\n");
  NACL_SYSCALL(null)();

  myprint("@write\n");
  myprint("hello worldn");

  myprint("@exit\n");
  NACL_SYSCALL(exit)(55);
  /* UNREACHABLE */
  return 0;
}
