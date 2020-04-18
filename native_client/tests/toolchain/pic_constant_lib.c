/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/toolchain/pic_constant_lib.h"

/*
 * This is a regression test for:
 *     https://code.google.com/p/nativeclient/issues/detail?id=3549
 * The compiler bug affected -fPIC compilations only.
 */

static const char *h(const char *s, size_t n) {
  return n != 0 ? g(s, n) : s;
}

static const char *f(const char *s) {
  return h(s, g(s, 0) - s);
}

const char *i(void) {
  return f("string constant");
}


/*
 * This is a regression test for:
 *     https://code.google.com/p/nativeclient/issues/detail?id=3598
 * x86_64-nacl-gcc with optimization and -fPIC had a bug wherein it would
 * attempt to generate an LEA instruction pattern using the full base
 * register + index register + displacement addressing mode in compiling
 * the trivial "sprint_nybble" function below, but then get an internal
 * compiler error (and crash) because a PIC reference to a static variable
 * is not usable as a displacement in an LEA instruction.
 */

static const char nybble[16][5]={
  "0000", "0001", "0010", "0011",
  "0100", "0101", "0110", "0111",
  "1000", "1001", "1010", "1011",
  "1100", "1101", "1110", "1111"
};

/*
 * The semantics for -fPIC require that a global function (with
 * default visibility) be called via the PLT, so making this global
 * truly ensures that main's call will not be inlined and so will be
 * compiled as the general-case version that elicits the code
 * pattern necessary to tickle the compiler bug.  Conversely, with
 * static and __attribute__((noinline)), while gcc-4.4 does what we
 * want, gcc-4.8 doesn't inline per se, but replaces the standalone
 * general-case function with a standalone (obeying noinline!)
 * fully constant-propagated-and-folded variant that doesn't even do
 * the table lookup at all!
 */
void sprint_nybble(int i, char s[4]) {
  const char *c = nybble[i & 0x0f];
  s[0] = c[0];
  s[1] = c[1];
  s[2] = c[2];
  s[3] = c[3];
}
