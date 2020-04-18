/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/toolchain/eh_helper.h"

class C {
 public:
  C() { next_step(4); }
  virtual ~C() { next_step(5); }
};


void tryit() {
  next_step(3);
  C c;
  throw 666;
  abort();
}


int main() {
  next_step(1);
  try {
    next_step(2);
    tryit();
  } catch(int x) {
    next_step(6);
    if (x != 666) abort();
  }

  next_step(7);
  return 55;
}
