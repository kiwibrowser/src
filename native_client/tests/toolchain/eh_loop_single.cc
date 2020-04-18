/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/toolchain/eh_helper.h"

int N = 10;

int main() {
  int i = -1;
  next_step(1);
  try {
    next_step(2);
    for (i = 0; i < N; ++i) {
      printf("iteration: %d\n", i);
      next_step(3);
      throw(666);
      abort();
    }
    abort();
  } catch(int x) {
    if (x != 666) abort();
    next_step(4);
    if (i != 0) abort();
    return 55;
  }

  return -1;
}
