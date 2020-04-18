/*
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "native_client/src/shared/platform/nacl_exit.h"

void NaClAbort(void) {
#ifdef COVERAGE
  /* Give coverage runs a chance to flush coverage data */
  exit(-SIGABRT);
#else
  abort();
#endif
}

void NaClExit(int err_code) {
#ifdef COVERAGE
  /* Give coverage runs a chance to flush coverage data */
  exit(err_code);
#else
  /*
   * Use _exit to exit immediately/
   */
  _exit(err_code);
#endif
}

