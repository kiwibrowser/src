/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/toolchain/eh_helper.h"

class A {
 public:
  A() { next_step(4); }
  ~A() { next_step(7); }
};


class B {
 public:
  B() { next_step(6); }
  ~B() { next_step(9);}
};


class C {
 public:
  C() { abort(); }
  ~C() { abort(); }
};


void inner() {
  next_step(3);
  A a;
  try {
    next_step(5);
    throw B();
    abort();
  } catch(C &) {
    abort();
  }
}


int main() {
  next_step(1);

  try {
    next_step(2);
    inner();
  } catch(...) {
    next_step(8);
  }
  next_step(10);

  return 55;
}
