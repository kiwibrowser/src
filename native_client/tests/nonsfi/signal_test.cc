/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"

static jmp_buf g_jmp_buf;

static void return_from_signal_handler(int signo) {
  ASSERT_EQ(signo, SIGSEGV);
  longjmp(g_jmp_buf, 42);
}

int main(int argc, char *argv[]) {
  sighandler_t prev_handler = signal(SIGSEGV, SIG_IGN);
  ASSERT_EQ(prev_handler, SIG_DFL);

  prev_handler = signal(SIGSEGV, return_from_signal_handler);
  ASSERT_EQ(prev_handler, SIG_IGN);

  int rc = setjmp(g_jmp_buf);
  if (rc == 0) {
    /* Crash. */
    *(volatile int *) 0 = 0;
    _exit(1); /* Shouldn't reach here. */
  }

  ASSERT_EQ(rc, 42);
  puts("PASSED");
  return 0;
}
