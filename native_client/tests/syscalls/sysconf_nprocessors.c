/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
  long nproc = sysconf(_SC_NPROCESSORS_ONLN);
  printf("sysconf(_SC_NPROCESSORS_ONLN) = %ld processors.\n", nproc);
  if (nproc < 0) {
    printf("errno = %d\n", errno);
  }
  return 0;
}
