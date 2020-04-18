/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl test for super simple program not using newlib
 */

#include "barebones.h"

/* NOTE: must not be const to prevent llvm optimizations */
int startval = 5;

static int foo0(void) { return 0; }
static int foo1(void) { return 1; }
static int foo2(void) { return 2; }
static int foo3(void) { return 3; }
static int foo4(void) { return 4; }
static int foo5(void) { return 55; }
static int foo6(void) { return 6; }
static int foo7(void) { return 7; }
static int foo8(void) { return 8; }
static int foo9(void) { return 9; }

typedef int (*myfuptr)(void);

static myfuptr vtable[] = {
  foo0,
  foo1,
  foo2,
  foo3,
  foo4,
  foo5,
  foo6,
  foo7,
  foo8,
  foo9,
};

int main(void) {
  int result = vtable[startval]();
  NACL_SYSCALL(exit)(result);
  /* UNREACHABLE */
  return 0;
}
