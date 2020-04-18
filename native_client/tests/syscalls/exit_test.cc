/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Test for exit_success syscall.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(const int argc, const char *argv[]) {
  int exit_value = 0;
  if (argc > 1) {
    exit_value = atoi(argv[1]);
  }

  printf("This program should exit and return %d.\n", exit_value);

  _exit(exit_value);
  printf("FAIL!  The program did not exit");
  return 1;
}
