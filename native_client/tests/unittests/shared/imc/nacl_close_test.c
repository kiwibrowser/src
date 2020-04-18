/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_error.h"

/* Writes the last error message to the standard error. */
static void failWithErrno(const char* message) {
  char buffer[256];

  if (0 == NaClGetLastErrorString(buffer, sizeof(buffer))) {
    fprintf(stderr, "%s: %s", message, buffer);
  }
  exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
  NaClHandle pair[2];

  UNREFERENCED_PARAMETER(argc);
  UNREFERENCED_PARAMETER(argv);

  if (0 != NaClSocketPair(pair)) {
    failWithErrno("SocketPair");
  }

  if (0 != NaClClose(pair[0])) {
    failWithErrno("NaClClose");
  }

  if (0 != NaClClose(pair[1])) {
    failWithErrno("NaClClose");
  }

  return 0;
}
