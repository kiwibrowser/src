/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/toolchain/eh_helper.h"

int N = 10;
/* We call next_step 4x per iteration */
int STRIDE = 4;

int main() {
  int i = 1;
  next_step(1);
  for (i = 0; i < N; ++i) {
    printf("iteration: %d\n", i);
    next_step(2 + i * STRIDE);
    try {
      next_step(3 + i * STRIDE);
      throw(666 + i);
      abort();
    } catch(int x) {
      if (x != 666 + i) abort();
      next_step(4 + i * STRIDE);
    }
    next_step(5 + i * STRIDE);
  }

  next_step(2 + N * STRIDE);
  return 55;
}
