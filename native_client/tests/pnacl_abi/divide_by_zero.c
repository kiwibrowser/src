/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

/*
 *  NOTE: these are volatile globals to keep a smart compiler (llvm)
 *       from treating this program as having undefined behavior.
 *       Description of behavior: http://llvm.org/bugs/show_bug.cgi?id=3828#c6
 */
volatile unsigned int denominator = 0;
volatile unsigned int numerator = 1;

int main(void) {
  unsigned char res = numerator / denominator;
  printf("FAIL: divide by zero resulted in %x rather than faulting\n", res);
  return 1;
}
