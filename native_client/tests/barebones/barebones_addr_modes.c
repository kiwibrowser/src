/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl test for super simple program not using newlib:
 * This test exercies addressing modes especially those
 * that would tempt the compiler to use reg+reg to compute the ea.
 */

#include "barebones.h"

/* NOTE: must not be const to prevent llvm optimizations */
volatile int startval = 20;


int global_array_int[100];
char global_array_char[100];


int main(int argc, char* argv[]) {
  int local_array_int[100];
  int local_array_char[100];
  int old_startval;
  int i;
  int result;

  /* put ones everywhere (with some precautions to confuse optimizers */
  for (i = 0 ; i < startval; ++i) {
    global_array_int[i] = startval - 19;
    local_array_int[i] = startval - 19;

    global_array_char[i] = startval - 19;
    local_array_char[i] = startval - 19;
  }

  /* put ones everywhere */
  /* copy ones around */
  for (i = 0 ; i < startval; ++i) {
    global_array_int[local_array_int[i]] =
      local_array_int[global_array_int[i]];
    global_array_char[(int)local_array_char[i]] =
      local_array_char[(int)global_array_char[i]];
  }

  /* more address magic */
  old_startval = startval;
  for (i = 0; i < old_startval; ++i) {
    global_array_int[startval] = global_array_int[i];
    local_array_int[startval] = local_array_int[i];

    global_array_char[startval] = global_array_char[i];
    global_array_char[startval + 1] = global_array_char[i];
    global_array_char[startval + 2] = global_array_char[i];
    global_array_char[startval + 3] = global_array_char[i];

    local_array_char[startval] = local_array_char[i];
    startval = local_array_int[startval];
  }

  /* make sure everything adds up to 55, our magic number */
  result = 15;
  for (i = 0; i < 10; ++i) {
    result += local_array_int[i];
    result += local_array_char[i];
    result += global_array_int[i];
    result += global_array_char[i];
  }

  NACL_SYSCALL(exit)(result);

  /* UNREACHABLE */
  return 0;
}
