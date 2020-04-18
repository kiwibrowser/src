/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <pthread.h>

#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"


/*
 * This tests the "second TLS" syscalls, which are reserved for the
 * use of the integrated runtime (IRT) library, so that the IRT can
 * have TLS that is separate from the user program.
 *
 * Of course, we are testing this from a "user program", so we are
 * assuming that we are not interfering with any IRT that may have
 * loaded this program.  This is true for now.
 */

void *test_thread(void *thread_arg) {
  /* The second TLS defaults to 0 in any new thread. */
  void *value = NACL_SYSCALL(second_tls_get)();
  assert(value == NULL);

  /* We should be able to set the TLS value and get the same value back. */
  int rc = NACL_SYSCALL(second_tls_set)((void *) 0x12345678);
  assert(rc == 0);
  value = NACL_SYSCALL(second_tls_get)();
  assert(value == (void *) 0x12345678);

  return NULL;
}

/*
 * We do the check at least twice: TLS should be independent for each
 * thread.
 */
#define THREAD_COUNT 5

int main(void) {
  pthread_t tids[THREAD_COUNT];
  int i;
  for (i = 0; i < THREAD_COUNT; i++) {
    int rc = pthread_create(&tids[i], NULL, test_thread, NULL);
    assert(rc == 0);
  }
  for (i = 0; i < THREAD_COUNT; i++) {
    int rc = pthread_join(tids[i], NULL);
    assert(rc == 0);
  }
  return 0;
}
