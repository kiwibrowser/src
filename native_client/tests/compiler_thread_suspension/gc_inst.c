/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/nacl_assert.h"
#include "native_client/tests/compiler_thread_suspension/gc_inst.h"


/* Make sure the function doesn't inline so we still get a prologue */
__attribute__((noinline))
void check_func_instrumentation(unsigned int arg) {
  ASSERT_LT(arg, thread_suspend_if_needed_count);
}

void test_compiler_instrumentation(void) {
  int i;
  /* Test function prologue instrumentation */
  unsigned int local_thread_suspend_count = thread_suspend_if_needed_count;
  check_func_instrumentation(local_thread_suspend_count);
  /* Check back-branch instrumentation */
  local_thread_suspend_count = thread_suspend_if_needed_count;
  i = 0;
  while (1) {
    /* Break after one iteration */
    if (i > 0)
      break;
    i++;
  }
  ASSERT_LT(local_thread_suspend_count, thread_suspend_if_needed_count);
}

int main(void) {
  test_compiler_instrumentation();
  return 0;
}
