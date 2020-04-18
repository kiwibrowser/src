/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>

#include "native_client/src/include/nacl_assert.h"


static void *test_stack_end(void *thread_arg) {
  void *stack_end = NULL;
  int rc = pthread_get_stack_end_np(pthread_self(), &stack_end);
  ASSERT_EQ(rc, 0);
  /* Check the result, assuming the stack grows downwards. */
  char var_on_stack;
  ASSERT_LT((uintptr_t) &var_on_stack, (uintptr_t) stack_end);
  /*
   * Sanity check assuming that this thread hasn't used more than 64k
   * of stack so far.
   */
  ASSERT_GE((uintptr_t) &var_on_stack, (uintptr_t) stack_end - 64 * 1024);
  /* Check that all of the stack is readable. */
  for (volatile char *p = &var_on_stack; p < (char *) stack_end; p++) {
    *p;
  }
  return NULL;
}

int main(void) {
  /* Test the initial thread. */
  test_stack_end(NULL);

  /* Test a new thread. */
  pthread_t tid;
  int rc = pthread_create(&tid, NULL, test_stack_end, NULL);
  ASSERT_EQ(rc, 0);
  rc = pthread_join(tid, NULL);
  ASSERT_EQ(rc, 0);

  return 0;
}
