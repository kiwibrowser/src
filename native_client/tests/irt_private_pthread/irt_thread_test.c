/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_interfaces.h"
#include "native_client/src/untrusted/nacl/tls.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"


__thread int tls_var = 123;

static pthread_t g_initial_thread_id;

/* This flag is set to zero by the IRT's thread_exit() function. */
static int32_t g_thread_flag;

static __thread uint32_t g_block_hook_call_count = 0;


static void wait_for_thread_exit(void) {
  /* Using a mutex+condvar is a hassle to set up, so wait by spinning. */
  while (g_thread_flag != 0) {
    sched_yield();
  }
}

static char *allocate_stack(void) {
  size_t stack_size = 0x10000;
  char *stack = malloc(stack_size);
  assert(stack != NULL);
  /* We assume that the stack grows downwards. */
  return stack + stack_size;
}

static void check_thread(void) {
  /* Check that IRT-internal TLS variables work inside this thread. */
  assert(tls_var == 123);
  tls_var = 456;

  /* NULL is not a valid thread ID in our pthreads implementation. */
  assert(pthread_self() != NULL);

  assert(pthread_self() != g_initial_thread_id);
  assert(!pthread_equal(pthread_self(), g_initial_thread_id));
}

static void pre_block_hook(void) {
  g_block_hook_call_count++;
}

static void post_block_hook(void) {
}

static int block_hooks_are_called(void) {
  uint32_t old_count = g_block_hook_call_count;
  /*
   * Call a syscall that should use NACL_GC_WRAP_SYSCALL.  In general,
   * read() can block, but it does not when we pass it these bad
   * arguments.
   */
  int result = read(-1, NULL, 0);
  /* Sanity checks. */
  assert(result == -1);
  assert(errno == EBADF);

  uint32_t count_diff = g_block_hook_call_count - old_count;
  assert(count_diff <= 1);
  return count_diff == 1;
}


static void user_thread_func(void) {
  check_thread();
  assert(block_hooks_are_called());
  nacl_irt_thread.thread_exit(&g_thread_flag);
}

void test_user_thread(void) {
  g_initial_thread_id = pthread_self();
  /* NULL is not a valid thread ID in our pthreads implementation. */
  assert(g_initial_thread_id != NULL);

  g_thread_flag = 1;
  void *dummy_tls = &dummy_tls;
  int rc = nacl_irt_thread.thread_create(user_thread_func, allocate_stack(),
                                         dummy_tls);
  assert(rc == 0);
  wait_for_thread_exit();
  /* The assignment should not have affected our copy of tls_var. */
  assert(tls_var == 123);
}


static void *irt_thread_func(void *arg) {
  assert((uintptr_t) arg == 0x12345678);
  check_thread();
  /* GC block hooks should not be called on IRT-internal threads. */
  assert(!block_hooks_are_called());
  return (void *) 0x23456789;
}

void test_irt_thread(void) {
  /*
   * Test that a thread created by the IRT for internal use works, and
   * that TLS works inside the thread.
   */
  pthread_t tid;
  int rc = pthread_create(&tid, NULL, irt_thread_func, (void *) 0x12345678);
  assert(rc == 0);

  void *result;
  rc = pthread_join(tid, &result);
  assert(rc == 0);
  assert(result == (void *) 0x23456789);
  /* The assignment should not have affected our copy of tls_var. */
  assert(tls_var == 123);
}


/* This waits for the initial thread to exit. */
static void initial_thread_exit_helper(void) {
  wait_for_thread_exit();
  _exit(0);
}

void test_exiting_initial_thread(void) {
  g_thread_flag = 1;
  void *dummy_tls = &dummy_tls;
  int rc = nacl_irt_thread.thread_create(initial_thread_exit_helper,
                                         allocate_stack(), dummy_tls);
  assert(rc == 0);
  nacl_irt_thread.thread_exit(&g_thread_flag);
  /* Should not reach here. */
  abort();
}


int main(void) {
  /*
   * Register these at startup because deregistering them is not
   * allowed by the IRT's interface and nor is registering different
   * functions afterwards.
   */
  int rc = nacl_irt_blockhook.register_block_hooks(pre_block_hook,
                                                   post_block_hook);
  assert(rc == 0);

  /*
   * There are three kinds of thread inside the IRT, which we test here.
   *
   * Type 1: user threads, created by the IRT's public thread_create()
   * interface.
   */
  printf("Running test_user_thread...\n");
  test_user_thread();

  /*
   * Type 2: IRT internal threads, created by the IRT's internal
   * pthread_create().
   */
  printf("Running test_irt_thread...\n");
  test_irt_thread();

  /*
   * Type 3: the initial thread
   */
  assert(block_hooks_are_called());
  printf("Running test_exiting_initial_thread...\n");
  test_exiting_initial_thread();
  /* This last test does not return. */
  return 1;
}


void _start(uint32_t *info) {
  __pthread_initialize();
  _exit(main());
}
