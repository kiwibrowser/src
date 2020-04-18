/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This test checks to see if using negative indexes cause problems.
 * (Regression test for an X86-64 sandboxing bug)
 */

#include "barebones.h"

int s = 110;
volatile unsigned arr[110];
void setarray(void);
void checkarray(void);

int main(int argc, char* argv[]) {
  setarray();
  checkarray();
  return 0;     /* unreachable */
}

void setarray(void) {
  register int i;
  for (i = s; i > 0; i--) {
    arr[s - i] = (i%2 ? 1 : 0);
  }
}

void checkarray(void) {
  register int i;
  int sum = 0;

  for (i = 0; i > -s; i--) {
    sum += arr[s + i - 1];
  }
  NACL_SYSCALL(exit)(sum);
}
