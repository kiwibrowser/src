/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/shared/platform/nacl_error.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"

#define MEMORY_SIZE (1 << 20)

/* Writes the last error message to the standard error. */
static void failWithErrno(const char* message) {
  char buffer[256];

  if (0 == NaClGetLastErrorString(buffer, sizeof(buffer))) {
    fprintf(stderr, "%s: %s\n", message, buffer);
  }
  exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
  NaClHandle handle;

  UNREFERENCED_PARAMETER(argc);
  UNREFERENCED_PARAMETER(argv);

  /* Check not executable shared memory. */
  handle = NaClCreateMemoryObject(MEMORY_SIZE, 0);
  if (NACL_INVALID_HANDLE == handle) {
    failWithErrno("NaClCreateMemoryObject(size, 0)");
  }
  if (0 != NaClClose(handle)) {
    failWithErrno("NaClClose(0)");
  }

  /* Check executable shared memory. */
  handle = NaClCreateMemoryObject(MEMORY_SIZE, 1);
#ifdef __native_client__
  if (NACL_INVALID_HANDLE != handle) {
    fprintf(stderr, "NaClCreateMemoryObject(size, 1) returned a valid shm. "
            "It must return -1\n");
    exit(EXIT_FAILURE);
  }
#else
  if (NACL_INVALID_HANDLE == handle) {
    failWithErrno("NaClCreateMemoryObject(size, 1)");
  }
  if (0 != NaClClose(handle)) {
    failWithErrno("NaClClose(1)");
  }
#endif

  return 0;
}
