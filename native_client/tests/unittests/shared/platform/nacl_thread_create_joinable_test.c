/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/shared/platform/nacl_error.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"


#define TEST_NUM 42

/* Writes the last error message to the standard error. */
static void failWithErrno(const char* message) {
  char buffer[256];

  if (0 == NaClGetLastErrorString(buffer, sizeof(buffer))) {
    fprintf(stderr, "%s: %s", message, buffer);
  }
  exit(EXIT_FAILURE);
}

void WINAPI myThread(void* arg) {
  int num;
  num = *((int*) arg);
  if (TEST_NUM != num) {
    fprintf(stderr, "myThread: %d expected, but %d received\n", TEST_NUM, num);
    exit(EXIT_FAILURE);
  }
  NaClThreadExit();
}

int main(int argc, char* argv[]) {
  int num = TEST_NUM;
  struct NaClThread thr;

  UNREFERENCED_PARAMETER(argc);
  UNREFERENCED_PARAMETER(argv);

  if (!NaClThreadCreateJoinable(&thr, myThread, &num, 128*1024)) {
    failWithErrno("NaClThreadCreateJoinable");
  }

  /* This test check that the thread actually starts. It checks that this does
   * not hang. */
  NaClThreadJoin(&thr);

  return 0;
}
