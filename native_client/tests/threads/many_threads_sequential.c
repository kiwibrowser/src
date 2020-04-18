/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>

/*
 * This test checks that we can successfully create and join threads
 * repeatedly, without any leaks that would cause this to eventually
 * fail.
 *
 * kIterations has to be higher than 8192 in order to test that
 * NaClTlsFree() gets called when untrusted threads exit.  8192 is the
 * maximum number of threads that can be created on x86-32 when %gs
 * has a different segment selector for each thread.
 */
const int kIterations = 10000;

void *thread_func(void *unused_arg) {
  return NULL;
}

int main(void) {
  int rc;
  int index;
  for (index = 0; index < kIterations; index++) {
    pthread_t tid;
    printf("creating thread %i\n", index);
    rc = pthread_create(&tid, NULL, thread_func, NULL);
    assert(rc == 0);
    rc = pthread_join(tid, NULL);
    assert(rc == 0);
  }
  return 0;
}
