/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "native_client/src/untrusted/valgrind/dynamic_annotations.h"

/*
 This small test is mainly for checking sanity of ThreadSanitizer on NaCl
 and is not a substitute for the ThreadSanitizer's full test suite.
 TODO(kcc): once ThreadSanitizer starts working reliably on NaCl,
 create a golden file to compare with the ThreadSanitizer's output.

 Tests named positive_* have races that should be detectable by ThreadSanitizer.
 negative_* tests don't have races and ThreadSanitizer should be silent.
*/

#define NOINLINE __attribute__((noinline))
#define SHOW_ME    printf("========== %s: =========\n", __FUNCTION__)

enum {
  kMaxNumThreads = 5
};

typedef void *(*ThreadCallback)(void*);

void create_and_join_threads(ThreadCallback *callbacks, int n) {
  int i;
  pthread_t t[kMaxNumThreads];
  int ids[kMaxNumThreads];
  assert(n <= kMaxNumThreads);
  for (i = 0; i < n; i++) {
    ids[i] = i;
    pthread_create(&t[i], NULL, callbacks[i], &ids[i]);
  }
  for (i = 0; i < n; i++) {
    pthread_join(t[i], NULL);
  }
}

/* Don't let various optimizations (e.g. tail call elimination) to happen. */
NOINLINE void break_optimization(void) {
#if defined(__pnacl__)
  volatile int foo;
  foo = 0;
#else
  __asm__ volatile ("");
#endif
}

/* -------------------------------------------------------------------------- */
/* positive_race_on_global_test
   Simle race on a global variable between three threads.
   We have few stack frames here to test ThreadSanitizer's output. */
int simple_race_obj;
NOINLINE void simple_race_write_frame_2(void) {
  break_optimization();
  simple_race_obj++;  /* Race here. */
}
NOINLINE void simple_race_write_frame_1(void) {
  break_optimization();
  simple_race_write_frame_2();
  break_optimization();
}
void *simple_race_write(void *arg) {
  break_optimization();
  simple_race_write_frame_1();
  break_optimization();
  return NULL;
}

void positive_race_on_global_test(void) {
  ThreadCallback t[] = {simple_race_write, simple_race_write,
    simple_race_write};
  SHOW_ME;
  ANNOTATE_EXPECT_RACE(&simple_race_obj,
      "positive_race_on_global_test. NACL_UNTRUSTED.");
  create_and_join_threads(t, 3);
}

/* -------------------------------------------------------------------------- */
/* positive_race_on_heap_test
   Simple race on a heap object. */
int *race_on_heap_obj;
NOINLINE void race_on_heap_frame_2(void) {
  break_optimization();
  *race_on_heap_obj = 1;  /* Race here. */
}
NOINLINE void race_on_heap_frame_1(void) {
  break_optimization();
  race_on_heap_frame_2();
  break_optimization();
}
void *race_on_heap_write(void *unused) {
  break_optimization();
  race_on_heap_frame_1();
  break_optimization();
  return NULL;
}

void positive_race_on_heap_test(void) {
  ThreadCallback t[2] = {race_on_heap_write, race_on_heap_write};
  SHOW_ME;
  race_on_heap_obj = (int*)malloc(sizeof(int));
  ANNOTATE_EXPECT_RACE(race_on_heap_obj,
      "positive_race_on_heap_test. NACL_UNTRUSTED.");
  create_and_join_threads(t, 2);
  free(race_on_heap_obj);
}

/* -------------------------------------------------------------------------- */
/* positive_wrong_lock_test
   Race on a global object between two threads, each holding its own lock. */
pthread_mutex_t wrong_lock_test_mu_1;
pthread_mutex_t wrong_lock_test_mu_2;
int wrong_lock_test_obj;

void *wrong_lock_test_access1(void *unused) {
  pthread_mutex_lock(&wrong_lock_test_mu_1);
  wrong_lock_test_obj++;  /* Race here. */
  pthread_mutex_unlock(&wrong_lock_test_mu_1);
  return NULL;
}

void *wrong_lock_test_access2(void *unused) {
  pthread_mutex_lock(&wrong_lock_test_mu_2);
  wrong_lock_test_obj++;  /* Race here. */
  pthread_mutex_unlock(&wrong_lock_test_mu_2);
  return NULL;
}

void positive_wrong_lock_test(void) {
  ThreadCallback t[2] = {wrong_lock_test_access1, wrong_lock_test_access2};
  pthread_mutex_init(&wrong_lock_test_mu_1, NULL);
  pthread_mutex_init(&wrong_lock_test_mu_2, NULL);
  SHOW_ME;
  ANNOTATE_EXPECT_RACE(&wrong_lock_test_obj,
      "positive_wrong_lock_test. NACL_UNTRUSTED.");
  create_and_join_threads(t, 2);
}

/* -------------------------------------------------------------------------- */
/* negative_locked_access_test
   Correctly synchronized code: an object is accessed
   by different threads under the same lock. */
pthread_mutex_t locked_access_test_mu;
int locked_access_test_obj;

void locked_access_test_frame_2(void) {
  pthread_mutex_lock(&locked_access_test_mu);
  locked_access_test_obj++;  /* No race here. */
  pthread_mutex_unlock(&locked_access_test_mu);
}
void locked_access_test_frame_1(void) {
  locked_access_test_frame_2();
}

void *locked_access_test_thread(void *unused) {
  locked_access_test_frame_1();
  return NULL;
}

void negative_locked_access_test(void) {
  ThreadCallback t[2] = {locked_access_test_thread, locked_access_test_thread};
  pthread_mutex_init(&locked_access_test_mu, NULL);
  SHOW_ME;
  create_and_join_threads(t, 2);
}

/* -------------------------------------------------------------------------- */
/* negative_posix_sem_test */

int     posix_sem_test_glob = 0;
sem_t   posix_sem_test_sem[2];

void* posix_sem_test_poster(void* unused) {
  posix_sem_test_glob = 1;
  sem_post(&posix_sem_test_sem[0]);
  sem_post(&posix_sem_test_sem[1]);
  return NULL;
}

void* posix_sem_test_waiter(void* unused) {
  sem_wait(&posix_sem_test_sem[0]);
  assert(posix_sem_test_glob == 1);
  return NULL;
}

void negative_posix_sem_test(void) {
  ThreadCallback t[2] = {posix_sem_test_poster, posix_sem_test_waiter};
  SHOW_ME;
  sem_init(&posix_sem_test_sem[0], 0, 0);
  sem_init(&posix_sem_test_sem[1], 0, 0);

  create_and_join_threads(t, 2);

  sem_destroy(&posix_sem_test_sem[0]);
  sem_destroy(&posix_sem_test_sem[1]);
}

/* -------------------------------------------------------------------------- */
/* pthread_create_and_join_test */
/* Test that thread creation and joining adds happens-before arcs. */

int thread_create_and_join_test_glob;

void* thread_create_and_join_test_thread1(void* unused) {
  thread_create_and_join_test_glob = 2;
  return NULL;
}

void thread_create_and_join_test(void) {
  ThreadCallback t[1] = {thread_create_and_join_test_thread1};

  SHOW_ME;
  ANNOTATE_TRACE_MEMORY(&thread_create_and_join_test_glob);

  thread_create_and_join_test_glob = 1;

  create_and_join_threads(t, 1);

  assert(thread_create_and_join_test_glob == 2);
}

/* -------------------------------------------------------------------------- */
int main(void) {
  if (1) positive_race_on_global_test();
  if (1) positive_race_on_heap_test();
  if (1) positive_wrong_lock_test();
  if (1) negative_locked_access_test();
  if (1) negative_posix_sem_test();
  if (1) thread_create_and_join_test();
  return 0;
}
