/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>


int main(void) {
  fprintf(stderr, "[CRASH_TEST] Causing crash in untrusted code...\n");
  /* We use "volatile" because otherwise LLVM gets too clever (at
     least on ARM) and generates an illegal instruction, which
     generates SIGILL instead of SIGSEGV, and LLVM also optimises away
     the rest of the function. */
  *(volatile int *) 0 = 0;
  fprintf(stderr, "[CRASH_TEST] FAIL: Survived crash attempt\n");
  return 1;
}
