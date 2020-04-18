/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_all_modules.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"


/*
 * This test case checks for an obscure gotcha in the Linux kernel.
 *
 * Some kernels do not assign to the top 2 bytes of the REG_CS array
 * entry when writing %cs to the stack.  If NaCl's signal handler does
 * not disregard the top 16 bits of REG_CS, it will incorrectly treat
 * a fault in trusted code as coming from untrusted code, and it will
 * crash while attempting to restore %gs.
 *
 * Specifically, this happens in 32-bit processes on the 64-bit kernel
 * from Ubuntu Hardy.
 *
 * See http://code.google.com/p/nativeclient/issues/detail?id=1486
 */

int main(void) {
  size_t stack_size = SIGSTKSZ;
  char *stack;
  stack_t st;
  int rc;

  NaClAllModulesInit();
  NaClSignalHandlerInit();

  /*
   * Allocate and register a signal stack, and ensure that it is
   * filled with non-zero bytes.
   */
  stack = malloc(stack_size);
  CHECK(stack != NULL);
  memset(stack, 0xff, stack_size);
  st.ss_size = stack_size;
  st.ss_sp = stack;
  st.ss_flags = 0;
  rc = sigaltstack(&st, NULL);
  CHECK(rc == 0);

  /*
   * Trigger a signal.  This should produce a "** Signal X from
   * trusted code" message, which the test runner checks for.
   */
  fprintf(stderr, "** intended_exit_status=trusted_segfault\n");

  /*
   * Clang transmutes a NULL pointer reference into a generic "undefined"
   * case.  That code crashes with a different signal than an actual bad
   * pointer reference, violating the tests' expectations.  A pointer that
   * is known bad but is not literally NULL does not get this treatment.
   */
  *(volatile int *) 1 = 0;

  fprintf(stderr, "Should never reach here.\n");
  return 1;
}
