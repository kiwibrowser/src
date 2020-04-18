/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

/* This is here so that we can set a breakpoint on it.  */
void foo(void) {
}

/* This is here so that we can set a breakpoint on it.  */
void bar(void) {
}

void *f1(void *arg) {
  foo();
  bar();
  return NULL;
}

void *f2(void *arg) {
  bar();
  foo();
  return NULL;
}

void test_break_continue_thread(void) {
  pthread_t t1;
  pthread_t t2;
  int rc;
  rc = pthread_create(&t1, NULL, f1, NULL);
  assert(rc == 0);
  rc = pthread_create(&t2, NULL, f2, NULL);
  assert(rc == 0);
  rc = pthread_join(t1, NULL);
  assert(rc == 0);
  rc = pthread_join(t2, NULL);
  assert(rc == 0);
}

/* This is here so that we can set a breakpoint on it.  */
void inside_f3(void) {
}

void *f3(void *arg) {
  /*
   * Wait until 'test_syscall_thread' thread runs 'pthread_join' and blocks
   * inside a syscall, then give GDB a chance to break.
   */
  int rc;
  rc = usleep(100000); /* Sleep for 0.1 seconds */
  assert(rc == 0);
  inside_f3();
  return NULL;
}

/*
 * Test that we have a meaningful backtrace when stopped in syscall.
 *
 * The problem here is to stop while executing a syscall:
 * - we can't set breakpoint inside a syscall, as we can't set breakpoints on
 *   trusted code;
 * - we can't send interrupt (ctrl+c) using GDB/MI, as this requires support
 *   for background execution we don't have;
 * Instead, we use an additional thread that waits until main thread is blocked
 * in syscall and then calls a function to hit a breakpoint.
 */
void test_syscall_thread(void) {
  pthread_t t;
  int rc;
  rc = pthread_create(&t, NULL, f3, NULL);
  assert(rc == 0);
  /* Run a syscall to be suspended by 'f3' thread.  */
  rc = pthread_join(t, NULL);
  assert(rc == 0);
}

int main(int argc, char **argv) {
  assert(argc >= 2);

  if (strcmp(argv[1], "break_continue_thread") == 0) {
    test_break_continue_thread();
    return 0;
  }
  if (strcmp(argv[1], "syscall_thread") == 0) {
    test_syscall_thread();
    return 0;
  }
  return 1;
}
