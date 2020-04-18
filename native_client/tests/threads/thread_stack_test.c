/*
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void CheckSuccess(int err, const char *filename, int lineno,
                         const char *expr) {
  if (err != 0) {
    printf("pthread/sem function failed with errno %i at %s:%i: %s\n",
           err, filename, lineno, expr);
    exit(1);
  }
}

static void Check(int cond, const char *filename, int lineno,
                  const char *expr) {
  if (!cond) {
    printf("condition failed at %s:%i: %s\n",
           filename, lineno, expr);
    exit(1);
  }
}

#define CHECK_OK(expr) (CheckSuccess((expr), __FILE__, __LINE__, #expr))
#define CHECK(expr) (Check((expr), __FILE__, __LINE__, #expr))

uintptr_t g_thread2_stack_start = 0;
uintptr_t g_thread2_stack_end = 0;

#define THREAD1_STACK_SIZE (128*1024)
#define THREAD2_STACK_SIZE (1024*1024)
#define TEST_ALLOCATION_SIZE (64*1024)

void* thread1_func(void* arg)
{
  pthread_exit(NULL);
  return NULL; /* Quiet compiler error */
}

struct thread2_arg {
  sem_t* thread2_started;
  sem_t* main_thread_done;
};

void* thread2_func(void* arg)
{
  int dummy = 0;
  const unsigned int kStackAlignment = 32;
  struct thread2_arg *args = (struct thread2_arg*)arg;

  /* Get a rough idea of the stack extents, may be off
   * by a few bytes but it's unimportant as long as we
   * don't over estimate the end and the begining isn't
   * under estimated by more than TEST_ALLOCATION_SIZE.
   */
  g_thread2_stack_end = (uintptr_t)&dummy;
  g_thread2_stack_end &= ~kStackAlignment;

  /* If we got the same stack as thread1, it's possible that
   * the stack for thread2 won't fit between 0x0 and the end.
   */
  CHECK(g_thread2_stack_end > THREAD2_STACK_SIZE);

  g_thread2_stack_start = g_thread2_stack_end - THREAD2_STACK_SIZE;

  CHECK_OK(sem_post(args->thread2_started));

  /* Keep thread alive while we check an allocation in the main thread. */
  CHECK_OK(sem_wait(args->main_thread_done));

  pthread_exit(NULL);
  return NULL; /* Quiet compiler error */
}

/* Make sure thread stacks are allocated correctly, not inherited from
 * previous threads when they don't fit.  This test creates a thread with
 * a small stack, waits for its exit, then creates a thread with a large
 * stack.  Then it allocates some memory to make sure a proper stack was
 * allocated.
 */

void TestThreadStackAllocation(void) {
  pthread_t thread1, thread2;
  pthread_attr_t attr1, attr2;
  sem_t thread2_started, main_thread_done;
  struct thread2_arg arg;

  CHECK_OK(pthread_attr_init(&attr1));
  CHECK_OK(pthread_attr_init(&attr2));

  CHECK_OK(pthread_attr_setstacksize(&attr1, THREAD1_STACK_SIZE));
  CHECK_OK(pthread_attr_setstacksize(&attr2, THREAD2_STACK_SIZE));

  CHECK_OK(pthread_create(&thread1, &attr1, thread1_func, NULL));
  CHECK_OK(pthread_join(thread1, NULL));

  CHECK_OK(sem_init(&thread2_started, 0, 0));
  CHECK_OK(sem_init(&main_thread_done, 0, 0));
  arg.thread2_started = &thread2_started;
  arg.main_thread_done = &main_thread_done;

  CHECK_OK(pthread_create(&thread2, &attr2, thread2_func, &arg));

  /* Wait for thread to spin up. */
  CHECK_OK(sem_wait(&thread2_started));

  uintptr_t test_alloc = (uintptr_t)malloc(TEST_ALLOCATION_SIZE);

  /* Check to make sure test_alloc doesn't lie in the stack of thread2. */
  CHECK(test_alloc < g_thread2_stack_start - TEST_ALLOCATION_SIZE ||
        test_alloc > g_thread2_stack_end);

  free((void*)test_alloc);

  /* Signal thread2 to exit. */
  CHECK_OK(sem_post(&main_thread_done));
  CHECK_OK(pthread_join(thread2, NULL));

  CHECK_OK(sem_destroy(&thread2_started));
  CHECK_OK(sem_destroy(&main_thread_done));
}


int main(int argc, char *argv[]) {

  /* Please don't add tests before TestThreadStackAllocation,
   * it could yield a false negative if the heap is fragmented.
   */
  TestThreadStackAllocation();

  return 0;
}
