/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/* NaCl inter-module communication primitives. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_error.h"

static const NaClSocketAddress test_address = {
  "google-imc-test"
};

static NaClHandle g_front;

static void CleanUp(void) {
  (void) NaClClose(g_front);
}

/* Writes the last error message to the standard error. */
void PrintError(const char* message) {
  char buffer[256];

  if (NaClGetLastErrorString(buffer, sizeof buffer) == 0) {
    fprintf(stderr, "%s: %s", message, buffer);
  }
}

int main(int argc, char* argv[]) {
  int result;
  NaClHandle pair[2];
  NaClMessageHeader header;
  NaClIOVec vec;
  char buffer[] = "Hello!";

  g_front = NaClBoundSocket(&test_address);
  if (g_front == NACL_INVALID_HANDLE) {
    PrintError("BoundSocket");
    exit(EXIT_FAILURE);
  }
  atexit(CleanUp);

  if (NaClSocketPair(pair) != 0) {
    PrintError("SocketPair");
    exit(EXIT_FAILURE);
  }

  vec.base = buffer;
  vec.length = sizeof buffer;

  /* Test SendDatagram */
  header.iov = &vec;
  header.iov_length = 1;
  header.handles = NULL;
  header.handle_count = 0;
  result = NaClSendDatagram(pair[0], &header, 0);
  assert(result == sizeof buffer);

  /* Test ReceiveDatagram */
  memset(buffer, 0, sizeof buffer);
  header.iov = &vec;
  header.iov_length = 1;
  header.handles = NULL;
  header.handle_count = 0;
  result = NaClReceiveDatagram(pair[1], &header, 0);
  assert(result == sizeof buffer);

  assert(strcmp(buffer, "Hello!") == 0);
  printf("%s\n", buffer);

  (void) NaClClose(pair[0]);
  (void) NaClClose(pair[1]);

  return 0;
}
