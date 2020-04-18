/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <semaphore.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/nacl/nacl_thread.h"

#define CHECK_OK(expr) ASSERT_EQ(expr, 0)

namespace {

struct nacl_irt_thread_v0_2 libnacl_irt_thread_v0_2;
struct nacl_irt_async_signal_handling libnacl_irt_async_signal_handling;

volatile int g_signal_count;
volatile int g_signal_arrived;
volatile int g_test_running;
nacl_irt_tid_t g_child_tid;
void *g_expected_tls;
sem_t g_sem;

int thread_create_wrapper(void (*start_func)(void), void *stack,
                          void *thread_ptr) {
  return libnacl_irt_thread_v0_2.thread_create(start_func, stack, thread_ptr,
                                               &g_child_tid);
}

int set_async_signal_handler(NaClIrtAsyncSignalHandler handler) {
  return libnacl_irt_async_signal_handling.set_async_signal_handler(handler);
}

int send_async_signal(nacl_irt_tid_t tid) {
  return libnacl_irt_async_signal_handling.send_async_signal(tid);
}

void safely_terminate_child() {
  /* Send a last signal to make sure any pending syscalls get interrupted. */
  int retval = send_async_signal(g_child_tid);
  if (retval != 0) {
    /* Thread might have exited before the signal is delivered. */
    ASSERT_EQ(retval, ESRCH);
  }
}

/*
 * Check that sending a signal before initializing signal support will result in
 * an error.
 */
void test_send_signal_before_set_handler() {
  int retval = send_async_signal(0);
  ASSERT_EQ(retval, ESRCH);
}

/*
 * Check that nacl_tls_get() is async-signal-safe.
 */
void tls_get_signal_handler(NaClExceptionContext *exc) {
  if (!g_test_running)
    return;
  ASSERT_EQ(nacl_tls_get(), g_expected_tls);
  g_signal_count++;
  g_signal_arrived = 1;
}

void *tls_get_thread_func(void *arg) {
  g_expected_tls = nacl_tls_get();
  CHECK_OK(sem_post(&g_sem));
  while (g_test_running) {
    ASSERT_EQ(nacl_tls_get(), g_expected_tls);
    if (__sync_bool_compare_and_swap(&g_signal_arrived, 1, 0)) {
      CHECK_OK(sem_post(&g_sem));
    }
  }
  return NULL;
}

void test_async_safe_tls_get() {
  CHECK_OK(sem_init(&g_sem, 0, 0));
  CHECK_OK(set_async_signal_handler(tls_get_signal_handler));

  pthread_t tid;
  g_signal_count = 0;
  g_signal_arrived = 0;
  g_test_running = true;
  CHECK_OK(pthread_create(&tid, NULL, tls_get_thread_func, NULL));

  CHECK_OK(sem_wait(&g_sem));
  const int kSignalCount = 1000;
  for (int i = 0; i < kSignalCount; i++) {
    CHECK_OK(send_async_signal(g_child_tid));
    CHECK_OK(sem_wait(&g_sem));
  }
  g_test_running = false;
  safely_terminate_child();
  CHECK_OK(pthread_join(tid, NULL));
  ASSERT_EQ(g_signal_count, kSignalCount);
  CHECK_OK(sem_destroy(&g_sem));
}

#if !defined(__arm__)
/* This test is broken on QEMU. */

/*
 * Check that both futex_wake() and futex_wait_abs() are signal-async-safe.
 */
void futex_signal_handler(NaClExceptionContext *exc) {
  int count = 0;
  ASSERT_EQ(__sync_bool_compare_and_swap(&g_signal_arrived, 0, 1), 1);
  CHECK_OK(__libnacl_irt_futex.futex_wake(&g_signal_arrived, INT_MAX, &count));
  /*
   * |count| is always 0 since the thread waiting is now running the signal
   * handler, so it did not actually count as a wakeup.
   */
  ASSERT_EQ(count, 0);
  if (g_test_running)
    g_signal_count++;
}

void *futex_thread_func(void *arg) {
  CHECK_OK(sem_post(&g_sem));
  struct timespec timeout;
  /*
   * Make the timeout be the current time plus 10 seconds. This timeout should
   * never kick in, but if it does it means we deadlocked, so it's better to
   * assert than letting the job itself time out.
   */
  clock_gettime(CLOCK_REALTIME, &timeout);
  timeout.tv_sec += 10;
  while (g_test_running) {
    int retval = __libnacl_irt_futex.futex_wait_abs(&g_signal_arrived, 0,
                                                    &timeout);
    if (retval == EWOULDBLOCK) {
      /*
       * The signal handler executed before we could wait and changed the value
       * of |g_signal_arrived|.
       */
    } else {
      /*
       * futex_wait_abs, when provided with a non-NULL timeout argument, can be
       * interrupted and will set errno to EINTR. This can happen even if the
       * SA_RESTART flag was used.
       */
      ASSERT_EQ(retval, EINTR);
    }
    ASSERT_EQ(__sync_bool_compare_and_swap(&g_signal_arrived, 1, 0), 1);
    /*
     * Have to test again since we could have gone sleeping again after the last
     * iteration.
     */
    if (g_test_running)
      CHECK_OK(sem_post(&g_sem));
  }
  return NULL;
}

void test_async_safe_futex() {
  CHECK_OK(sem_init(&g_sem, 0, 0));
  CHECK_OK(set_async_signal_handler(futex_signal_handler));

  pthread_t tid;
  g_signal_count = 0;
  g_signal_arrived = 0;
  g_test_running = true;
  CHECK_OK(pthread_create(&tid, NULL, futex_thread_func, NULL));

  CHECK_OK(sem_wait(&g_sem));
  const int kSignalCount = 1000;
  for (int i = 0; i < kSignalCount; i++) {
    CHECK_OK(send_async_signal(g_child_tid));
    CHECK_OK(sem_wait(&g_sem));
  }
  g_test_running = false;
  safely_terminate_child();
  CHECK_OK(pthread_join(tid, NULL));
  ASSERT_EQ(g_signal_count, kSignalCount);
  CHECK_OK(sem_destroy(&g_sem));
}

#endif

/*
 * Check that futex_wait_abs() with no timeout is restarted.
 * As opposed to the above test with futex, the signal handler does not try to
 * wake the thread up, since it will sometimes be called _after_ the
 * futex_wait_abs() returns.
 */
void futex_wait_signal_handler(NaClExceptionContext *exc) {
  ASSERT_EQ(__sync_bool_compare_and_swap(&g_signal_arrived, 0, 1), 1);
}

void *futex_wait_thread_func(void *arg) {
  volatile int *futex = (volatile int *)arg;
  CHECK_OK(sem_post(&g_sem));
  while (g_test_running) {
    /*
     * Unfortunately, Linux sometimes can return 0 (instead of EINTR) on
     * futex_wait_abs() when it is spuriously woken up.
     */
    while (*futex == 0) {
      int retval = __libnacl_irt_futex.futex_wait_abs(futex, 0, NULL);
      if (retval != EWOULDBLOCK)
        ASSERT_EQ(retval, 0);
    }
    ASSERT_EQ(__sync_bool_compare_and_swap(futex, 1, 0), 1);

    /*
     * Have to test again since we could have gone sleeping again after the last
     * iteration.
     */
    if (g_test_running) {
      ASSERT_EQ(__sync_bool_compare_and_swap(&g_signal_arrived, 1, 0), 1);
      g_signal_count++;
      CHECK_OK(sem_post(&g_sem));
    }
  }
  return NULL;
}

void test_futex_wait_restart() {
  CHECK_OK(sem_init(&g_sem, 0, 0));
  CHECK_OK(set_async_signal_handler(futex_wait_signal_handler));

  pthread_t tid;
  g_signal_count = 0;
  g_signal_arrived = 0;
  volatile int futex = 0;
  g_test_running = true;
  CHECK_OK(pthread_create(&tid, NULL, futex_wait_thread_func, (void *)&futex));

  CHECK_OK(sem_wait(&g_sem));
  const int kSignalCount = 1000;
  int count = 0;
  for (int i = 0; i < kSignalCount; i++) {
    /* Yield to the other process to try and get it in the desired state. */
    sched_yield();
    CHECK_OK(send_async_signal(g_child_tid));
    sched_yield();

    /* Wake it up using futex. This time, |count| may be 1. */
    ASSERT_EQ(__sync_bool_compare_and_swap(&futex, 0, 1), 1);
    CHECK_OK(__libnacl_irt_futex.futex_wake(&futex, INT_MAX, &count));
    ASSERT_LE(count, 1);

    CHECK_OK(sem_wait(&g_sem));
  }
  g_test_running = false;
  /*
   * Wake the thread up again in case it waited again.
   */
  __sync_bool_compare_and_swap(&futex, 0, 1);
  CHECK_OK(__libnacl_irt_futex.futex_wake(&futex, INT_MAX, &count));
  CHECK_OK(pthread_join(tid, NULL));
  ASSERT_EQ(g_signal_count, kSignalCount);
  CHECK_OK(sem_destroy(&g_sem));
}

/*
 * Check that send_async_signal() is async-signal-safe.
 */
void signal_signal_handler(NaClExceptionContext *exc) {
  if (!g_test_running)
    return;
  if (++g_signal_count % 2 == 1) {
    CHECK_OK(send_async_signal(g_child_tid));
    g_signal_arrived = 1;
  }
}

void *signal_thread_func(void *arg) {
  CHECK_OK(sem_post(&g_sem));
  struct timespec req, rem;
  /*
   * In case we are unlucky and the signal arrives before the first sleep, limit
   * the time sleeping to 10 msec.
   */
  req.tv_sec = 0;
  req.tv_nsec = 10000000;
  while (g_test_running) {
    while (g_test_running && !g_signal_arrived) {
      int retval = nanosleep(&req, &rem);
      if (retval != 0)
        ASSERT_EQ(errno, EINTR);
    }
    /*
     * Have to test again since we could have gone sleeping again after the last
     * iteration.
     */
    if (!g_test_running)
      break;
    g_signal_arrived = 0;
    CHECK_OK(sem_post(&g_sem));
  }
  return NULL;
}

void test_async_safe_signal() {
  CHECK_OK(sem_init(&g_sem, 0, 0));
  CHECK_OK(set_async_signal_handler(signal_signal_handler));

  pthread_t tid;
  g_test_running = true;
  g_signal_count = 0;
  g_signal_arrived = 0;
  CHECK_OK(pthread_create(&tid, NULL, signal_thread_func, NULL));

  CHECK_OK(sem_wait(&g_sem));
  const int kSignalCount = 1000;
  for (int i = 0; i < kSignalCount; i++) {
    CHECK_OK(send_async_signal(g_child_tid));
    CHECK_OK(sem_wait(&g_sem));
  }
  g_test_running = false;
  safely_terminate_child();
  CHECK_OK(pthread_join(tid, NULL));
  ASSERT_EQ(g_signal_count, 2 * kSignalCount);
  CHECK_OK(sem_destroy(&g_sem));
}

/*
 * Check that passing 0 as |tid| to send_async_signal() works and
 * sends a signal to the main thread.
 */
void main_signal_handler(NaClExceptionContext *exc) {
  g_signal_count = 1;
}

void test_main_signal() {
  CHECK_OK(set_async_signal_handler(main_signal_handler));

  g_signal_count = 0;
  CHECK_OK(send_async_signal(NACL_IRT_MAIN_THREAD_TID));
  ASSERT_EQ(g_signal_count, 1);
}

void run_test(const char *test_name, void (*test_func)(void)) {
  printf("Running %s...\n", test_name);
  test_func();
}

}  // namespace

#define RUN_TEST(test_func) (run_test(#test_func, test_func))

int main(void) {
  size_t bytes;
  bytes = nacl_interface_query(NACL_IRT_THREAD_v0_2, &libnacl_irt_thread_v0_2,
                               sizeof(libnacl_irt_thread_v0_2));
  ASSERT_EQ(bytes, sizeof(libnacl_irt_thread_v0_2));

  bytes = nacl_interface_query(NACL_IRT_ASYNC_SIGNAL_HANDLING_v0_1,
                               &libnacl_irt_async_signal_handling,
                               sizeof(libnacl_irt_async_signal_handling));
  ASSERT_EQ(bytes, sizeof(libnacl_irt_async_signal_handling));

  /*
   * In order to avoid modifying the libpthread implementation to save the
   * native tid, wrap that functionality so the tid is stored in a global
   * variable.
   */
  __libnacl_irt_thread.thread_create = &thread_create_wrapper;

  RUN_TEST(test_send_signal_before_set_handler);

  RUN_TEST(test_async_safe_tls_get);
#if !defined(__arm__)
  /*
   * Signals are sometimes delivered after the futex_wait syscall returns (as
   * opposed to interrupting it), which breaks this test.
   *
   * This problem only seems to happen in QEMU.
   */
  RUN_TEST(test_async_safe_futex);
#endif
  RUN_TEST(test_futex_wait_restart);
  RUN_TEST(test_async_safe_signal);
  RUN_TEST(test_main_signal);

  printf("Done\n");

  return 0;
}
