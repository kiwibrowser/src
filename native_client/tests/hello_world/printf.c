/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
   It can't get much simpler than this (uh, except for noop.c).
*/

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  int i;

  for (i = 1; i < argc; ++i) {
    long x;
    printf("%s\n", argv[i]);

    x = strtol(argv[i], 0, 0);
    printf("%ld\n", x);
    printf("%lx\n", x);
    printf("0x%08lx\n", x);
    printf("\n");
  }
  return 0;
}
