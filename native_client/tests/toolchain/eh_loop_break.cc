/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/toolchain/eh_helper.h"

int main() {
  next_step(1);
  for (;;) {
    next_step(2);
    try {
      next_step(3);
      throw(666);
      abort();
    } catch(int x) {
      if (x != 666) abort();
      next_step(4);
      break;
    }
    abort();
  }

  next_step(5);
  return 55;
}
