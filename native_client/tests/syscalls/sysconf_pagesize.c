/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <unistd.h>

int main(void) {
  long rv = sysconf(_SC_PAGESIZE);
  printf("%ld\n", rv);
  return rv != (1<<16);
}
