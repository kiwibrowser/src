/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <math.h>
#include <stdio.h>

/* Picking a base that can be represented exactly in FP, even when squared */
volatile float fnum_base = 4.125;
volatile float fnum_two = 2.0;
volatile int inum_two = 2;

volatile double dnum_base = 16.5;
volatile double dnum_two = 2.0;

int main(void) {
  printf("%f\n", pow(fnum_base, fnum_two));
  printf("%f\n", pow(fnum_base, inum_two));

  /* Only 2 digits after the dot are needed, but avoid appending 0s */
  printf("%.2f\n", pow(dnum_base, dnum_two));
  printf("%.2f\n", pow(dnum_base, inum_two));

  return 0;
}
