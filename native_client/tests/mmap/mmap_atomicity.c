/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>


void *mapping;

void *thread_func(void *unused_arg) {
  fprintf(stderr, "child thread has started.\n");
  while (1) {
    /*
     * This checks whether mmap() with MAP_FIXED is atomic.  On
     * Windows, mmap() must temporarily unmap and then re-map a page.
     * If this is observable by untrusted code, then the memory access
     * below can fault if it occurs while the address is temporarily
     * unmapped.
     * See http://code.google.com/p/nativeclient/issues/detail?id=1848
     *
     * This is technically a stress test: since this is
     * non-deterministic, we are not guaranteed to detect the problem
     * if it exists.  However, this is such a tight loop that if we
     * are running on a multicore system, we are almost certain to
     * detect the problem.
     */
    (*(volatile int *) mapping)++;
  }
  return NULL;
}

int main(void) {
  mapping = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE,
                 MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert(mapping != MAP_FAILED);

  pthread_t tid;
  int rc = pthread_create(&tid, NULL, thread_func, NULL);
  assert(rc == 0);

  /*
   * To increase the chance of detecting a problem, spin until we can
   * see that our thread has really been scheduled.
   */
  fprintf(stderr, "waiting for child thread...\n");
  while (*(volatile int *) mapping == 0) {
    /* Nothing */
  }

  for (int index = 0; index < 1000; index++) {
    fprintf(stderr, "mmap call #%i\n", index);
    void *result = mmap(mapping, 0x10000, PROT_READ | PROT_WRITE,
                        MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    assert(result == mapping);

    /*
     * Sanity check: Spin until we see that our thread has touched the
     * page.  This checks that the thread has been correctly
     * unsuspended on Windows.  On failure, this will hang.
     */
    fprintf(stderr, "checking for write to page...\n");
    int value = *(volatile int *) mapping;
    while (*(volatile int *) mapping == value) {
      /* Nothing */
    }
  }
  return 0;
}
