/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

int volatile XXX;

void wo(void) {
  XXX = 10;
  printf("Wo");
}

void r(void) {
  printf("r");
}

void l(void) {
  printf("l");
}

void d(void) {
  printf("d\n");
  XXX = 0;
}

int main(int argc, char* argv[]) {
  /* Test that the printf does not get converted to puts() after
   * link time, which would create an undefined symbol.  We need more
   * than one call to printf, so that the optimization passes do not
   * specialize printf or inline it instead, and skip the puts() optimization.
   */
  printf("Hello ");
  wo();
  /* It could also be printf -> putchar. */
  r();
  l();
  d();
  return XXX;
}
