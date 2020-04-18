/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"

static struct nacl_irt_fdio g_fdio;

void init_error_report_module(void) {
  size_t bytes = nacl_interface_query(NACL_IRT_FDIO_v0_1,
                                      &g_fdio, sizeof(g_fdio));
  ASSERT_EQ(bytes, sizeof(g_fdio));
}

void irt_ext_test_print(const char *format, ...) {
  char buffer[512];
  int bufferlen;
  size_t nwrote;

  va_list args;
  va_start (args, format);
  bufferlen = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
  va_end (args);

  if (bufferlen > 0) {
    int ret = g_fdio.write(STDERR_FILENO, buffer, bufferlen, &nwrote);
    if (ret != 0 || nwrote != bufferlen) {
      __builtin_trap();
    }
  }
}
