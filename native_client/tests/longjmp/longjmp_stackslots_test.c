/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * This is a regression test for a bug in PNaCl's setjmp() implementation.
 * The PNaCl translator was reusing stack slots in cases where use of
 * setjmp() should disable that.  See:
 * https://code.google.com/p/nativeclient/issues/detail?id=3733
 *
 * To test for the bug, this must be compiled with optimisation, in order
 * to run mem2reg and use spill slots.  However, get_next() and
 * check_equals() must not be inlined.
 */

static int g_counter = 0;

__attribute__((noinline))
int get_next(void) {
  return ++g_counter;
}

__attribute__((noinline))
void check_equals(int actual, int expected) {
  if (actual == expected) {
    printf("OK: got %i\n", actual);
  } else {
    printf("ERROR: got %i but expected %i\n", actual, expected);
    exit(1);
  }
}

int main(void) {
  /*
   * Keep enough variables live across the setjmp() call that they don't
   * all fit in registers, so that the compiler spills some of them to
   * spill slots on the stack.
   *
   * There need to be >8 variables here to catch the bug on ARM (without
   * NaCl SFI), because ARM's calling conventions use 8 callee-saved
   * registers (r4-r11), not counting the stack pointer.
   */
  int a1 = get_next();
  int a2 = get_next();
  int a3 = get_next();
  int a4 = get_next();
  int a5 = get_next();
  int a6 = get_next();
  int a7 = get_next();
  int a8 = get_next();
  int a9 = get_next();
  int a10 = get_next();
  int a11 = get_next();
  int a12 = get_next();
  int a13 = get_next();
  int a14 = get_next();
  int a15 = get_next();
  int a16 = get_next();
  jmp_buf buf;
  if (setjmp(buf)) {
    check_equals(a1, 1);
    check_equals(a2, 2);
    check_equals(a3, 3);
    check_equals(a4, 4);
    check_equals(a5, 5);
    check_equals(a6, 6);
    check_equals(a7, 7);
    check_equals(a8, 8);
    check_equals(a9, 9);
    check_equals(a10, 10);
    check_equals(a11, 11);
    check_equals(a12, 12);
    check_equals(a13, 13);
    check_equals(a14, 14);
    check_equals(a15, 15);
    check_equals(a16, 16);
    return 0;
  }
  /*
   * Again, keep enough variables live that some of them will need spill
   * slots.  A correct compiler will realise that a1...aN are still live
   * (via setjmp()+longjmp()), and so not reuse the earlier spill slots.
   * An incorrect compiler will think that a1..aN are dead here and wrongly
   * reuse the earlier spill slots.
   */
  int b1 = get_next();
  int b2 = get_next();
  int b3 = get_next();
  int b4 = get_next();
  int b5 = get_next();
  int b6 = get_next();
  int b7 = get_next();
  int b8 = get_next();
  int b9 = get_next();
  int b10 = get_next();
  int b11 = get_next();
  int b12 = get_next();
  int b13 = get_next();
  int b14 = get_next();
  int b15 = get_next();
  int b16 = get_next();
  int start = 16;
  check_equals(b1, start + 1);
  check_equals(b2, start + 2);
  check_equals(b3, start + 3);
  check_equals(b4, start + 4);
  check_equals(b5, start + 5);
  check_equals(b6, start + 6);
  check_equals(b7, start + 7);
  check_equals(b8, start + 8);
  check_equals(b9, start + 9);
  check_equals(b10, start + 10);
  check_equals(b11, start + 11);
  check_equals(b12, start + 12);
  check_equals(b13, start + 13);
  check_equals(b14, start + 14);
  check_equals(b15, start + 15);
  check_equals(b16, start + 16);
  longjmp(buf, 1);
}
