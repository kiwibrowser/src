/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This test checks exiting main thread before other threads are finished.
 * Also this checks whether joining with main thread is working.
 */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

volatile pthread_t tmain;

void* ThreadFunction(void* arg) {
  int rv;
  rv = pthread_join(tmain, NULL);
  if (rv != 0) {
    fprintf(stderr, "Error: can't join with main thread %d", rv);
    exit(3);
  }
  return NULL;
}

int main(void) {
  pthread_t tid;
  int rv;
  tmain = pthread_self();
  rv = pthread_create(&tid, NULL, ThreadFunction, NULL);
  if (rv != 0) {
    fprintf(stderr, "Error: in thread creation %d\n", rv);
    return 1;
  }
  pthread_exit(NULL);
  /* Should never get here. */
  return 2;
}
