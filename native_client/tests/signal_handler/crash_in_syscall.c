/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/trusted/service_runtime/include/sys/nacl_test_crash.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"


int main(void) {
  /*
   * There are two possible kinds of fault in trusted code:
   *  1) Faults legitimately triggered by untrusted code.  These are
   *     safe and do not indicate a bug.
   *  2) Faults caused by bugs in trusted code.
   *
   * Currently the signal handler does not distinguish between the
   * two.  Ultimately, we do want to distinguish between them in order
   * to avoid misleading bug reports.
   * See http://code.google.com/p/nativeclient/issues/detail?id=579
   * Below, we trigger a memory access fault in trusted code, inside a
   * syscall handler.
   */
  fprintf(stderr, "About to crash\n");
  fflush(stderr);
  NACL_SYSCALL(test_crash)(NACL_TEST_CRASH_MEMORY);
  printf("We do not expect to reach here.\n");
  return 1;
}
