/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"


class ClassWithInitializer {
 public:
  ClassWithInitializer() {
    // Sleep for 0.1 seconds.  This should be long enough for both threads
    // to enter the static initializer at the same time.
    struct timespec time = { 0, 100000000 };
    int rc = nanosleep(&time, NULL);
    ASSERT_EQ(rc, 0);
  }
};

static void *thread_func(void *arg) {
  static ClassWithInitializer obj;
  printf("Initialised object %p\n", (void *) &obj);
  return NULL;
}

int main() {
  pthread_t tid;
  int rc = pthread_create(&tid, NULL, thread_func, NULL);
  ASSERT_EQ(rc, 0);

  thread_func(NULL);

  rc = pthread_join(tid, NULL);
  ASSERT_EQ(rc, 0);
  return 0;
}
