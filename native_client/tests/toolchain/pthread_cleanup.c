/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void *(*thread_func_t)(void *arg);

static int test_variables[] = {0, 0, 0};
static int test_variables_expect[] = {1, 2, 0};
/*
 * Test both that pthread_cleanup_push/pop works as it should, and that it
 * doesn't clobber nearby stack, as seen in
 * http://code.google.com/p/nativeclient/issues/detail?id=2490
 */
static void cleanup0(void *arg) {
  *(int *) arg = 1;
}

#define GUARD_MAGIC 0xdeadbeef

static void *thread_func(void *arg) {
  volatile int guard = GUARD_MAGIC;
  pthread_cleanup_push(&cleanup0, arg);
  pthread_cleanup_pop(1);
  if (guard != GUARD_MAGIC) {
    fprintf(stderr, "guard clobbered to %#x\n", guard);
    exit(2);
  }
  return NULL;
}

/*
 * Test that on pthread_exit() call cleanup handler is called.
 */
static void cleanup1(void *arg) {
  *(int *) arg = 2;
}

static void *thread_func_exit(void *arg) {
  pthread_cleanup_push(&cleanup1, arg);
  pthread_exit(NULL);
  pthread_cleanup_pop(0);
  return NULL;
}

/*
 * Test that on normal return cleanup handler is *not* called,
 * although the standard says this is unspecified behaviour.
 */
static void cleanup2(void *arg) {
  *(int *) arg = 3;
}

static void *thread_func_noexit(void *arg) {
  pthread_cleanup_push(&cleanup2, arg);
  return NULL;
  pthread_cleanup_pop(0);
  return NULL;
}

int main(void) {
  thread_func_t test_functions[] = {
    &thread_func,
    &thread_func_exit,
    &thread_func_noexit,
    NULL
  };
  int i;

  for (i = 0; test_functions[i] != NULL; i++) {
    pthread_t th;
    int error = pthread_create(&th, NULL,
                               test_functions[i], &test_variables[i]);
    if (error != 0) {
      fprintf(stderr, "pthread_create: %s\n", strerror(error));
      return 1;
    }
    error = pthread_join(th, NULL);
    if (error != 0) {
      fprintf(stderr, "pthread_join: %s\n", strerror(error));
      return 1;
    }

    if (test_variables[i] != test_variables_expect[i]) {
      fprintf(stderr, "pthread_cleanup: doesn't work %d != %d\n",
              test_variables[i], test_variables_expect[i]);
      return 2;
    }
  }

  return 0;
}
