/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Test basic floating point operations
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#define NUM_ARRAY_ELEMENTS 3

float Numbers1[NUM_ARRAY_ELEMENTS] = {0.5, 0.3, 0.2};


int main(int argc, char* argv[]) {
  int i;
  float sum;

  /* This will not happen but helps confusing the optimizer */
  if (argc > 1000) {
    for (i = 0; i < NUM_ARRAY_ELEMENTS; ++i) {
      Numbers1[i] = (float) argv[i][0];
    }
  }

    /* actual computation */
  sum = 0.0;
  for (i = 0; i < NUM_ARRAY_ELEMENTS; ++i) {
    sum += Numbers1[i];
  }

  assert (fabs (sum - 1.) < 0.001);
  return 0;
}
