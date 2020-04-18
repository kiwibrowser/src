/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/trusted/debug_stub/platform.h"
#include "native_client/src/trusted/debug_stub/test.h"

int main(int argc, const char *argv[]) {
  int errs = 0;

  (void) argc;
  (void) argv;

  printf("Testing Utils.\n");
  errs += TestUtil();

  printf("Testing ABI.\n");
  errs += TestAbi();

  printf("Testing Packets.\n");
  errs += TestPacket();

  printf("Testing Session.\n");
  errs += TestSession();

  if (errs) printf("FAILED with %d errors.\n", errs);
  return errs;
}

