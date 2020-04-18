/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *thread(void *ignored) {
  while (1)
    sched_yield();
  return NULL;
}

int main(void) {
  pthread_t tid;
  int rc = pthread_create(&tid, NULL, thread, NULL);
  if (rc) {
    fprintf(stderr, "pthread_create: %s\n", strerror(errno));
    return 1;
  }
  /*
   * Returning now calls exit, which calls atexit hooks.
   * This all should work fine despite the existence of the other thread.
   * With http://code.google.com/p/nativeclient/issues/detail?id=581
   * it would hang waiting for the thread to die, which is wrong semantics.
   */
  return 0;
}
