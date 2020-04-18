/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
*/

/*
  Test Valgrind leak checker sanity.

  The idea is to leak a block of an unusual size (to avoid possible collisions)
  and check that it appears in Valgrind report. We ignore all other leaks in
  this test.
*/

#include <stdlib.h>

int main(void) {
  /* Obtained with a fair /dev/random read. Guaranteed to be random. */
  malloc(7931147);
  return 0;
}
