/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>

void hello_world(void) {
  printf("Hello, World!\n");
}

int main(int argc, char* argv[]) {
  int i;
  for (i=0; i< 10; ++i) {
    atexit(hello_world);
  }
  return 0;
}
