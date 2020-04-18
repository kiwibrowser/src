/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <string.h>

#include "native_client/src/trusted/service_runtime/include/sys/nacl_nice.h"
#include "native_client/tests/irt_ext/libc/libc_test.h"
#include "native_client/tests/irt_ext/threading.h"

typedef int (*TYPE_thread_test)(struct threading_environment *env);

#define TEST_TIME_VALUE 20
#define TEST_WAIT_TIME 10

static void *nop_thread(void *arg) {
  return NULL;
}

#ifndef __GLIBC__
static void *priority_thread(void *arg) {
  const int prio = (int) arg;
  pthread_t this_thread = pthread_self();
  pthread_setschedprio(this_thread, prio);
  return NULL;
}

static void *mutex_thread(void *arg) {
  pthread_mutex_t *mutex = arg;

  int ret = pthread_mutex_lock(mutex);
  if (0 != ret)
    return (void *) ((uintptr_t) ret);

  return (void *) ((uintptr_t) pthread_mutex_unlock(mutex));
}

struct semaphore_thread_args {
  volatile int finished;
  sem_t *semaphore;
};

static void *semaphore_thread(void *arg) {
  struct semaphore_thread_args *semaphore_args = arg;
  int ret = sem_wait(semaphore_args->semaphore);
  semaphore_args->finished = 1;
  return (void *) ((uintptr_t) ret);
}

enum CondWaitState {
  /* Initial state. */
  CondWaitStateInitialized,

  /* Set by the cond_wait_thread after obtaining the lock. */
  CondWaitStateReady,

  /* Set by the main thread to indicate it is signally the condition. */
  CondWaitStateSignalling,

  /* Set by the cond_wait_thread to signify the condition is signalled. */
  CondWaitStateSignalled,
};

struct CondWaitThreadArg {
  volatile enum CondWaitState state;
  pthread_mutex_t *mutex;
  pthread_cond_t *cond;
};

static void *cond_wait_thread(void *arg) {
  struct CondWaitThreadArg *cond_wait_arg = arg;

  int ret = pthread_mutex_lock(cond_wait_arg->mutex);
  if (0 != ret)
    return (void *) ((uintptr_t) ret);

  cond_wait_arg->state = CondWaitStateReady;

  ret = pthread_cond_wait(cond_wait_arg->cond, cond_wait_arg->mutex);
  if (0 != ret)
    return (void *) ((uintptr_t) ret);

  cond_wait_arg->state = CondWaitStateSignalled;

  return (void *) ((uintptr_t) pthread_mutex_unlock(cond_wait_arg->mutex));
}
#endif

/* Basic pthread tests. */
static int do_thread_creation_test(struct threading_environment *env) {
  if (env->num_threads_created != 0 || env->num_threads_exited != 0) {
    irt_ext_test_print("do_thread_creation_test: Env is not initialized.\n");
    return 1;
  }

  pthread_t thread_id;
  int ret = pthread_create(&thread_id, NULL, nop_thread, NULL);
  if (0 != ret) {
    irt_ext_test_print("do_thread_creation_test: pthread_create failed: %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_join(thread_id, NULL);
  if (0 != ret) {
    irt_ext_test_print("do_thread_creation_test: pthread_join failed: %s\n",
                       strerror(ret));
    return 1;
  }

  if (env->num_threads_created != 1) {
    irt_ext_test_print("do_thread_creation_test: Thread Creation was not"
                       " recorded in the environment.\n");
    return 1;
  }

  if (env->num_threads_exited != 1) {
    irt_ext_test_print("do_thread_creation_test: Thread Exit was not"
                       " recorded in the environment.\n");
    return 1;
  }

  return 0;
}

static int do_thread_priority_test(struct threading_environment *env) {
  /*
   * TODO(mcgrathr): Implement pthread_setschedprio in NaCl glibc.
   */
#ifndef __GLIBC__
  if (env->last_set_thread_nice != 0) {
    irt_ext_test_print("do_thread_priority_test: Env is not initialized.\n");
    return 1;
  }

  pthread_t thread_id;
  int ret = pthread_create(&thread_id, NULL, priority_thread,
                           (void *) NICE_REALTIME);
  if (0 != ret) {
    irt_ext_test_print("do_thread_priority_test: pthread_create failed: %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_join(thread_id, NULL);
  if (0 != ret) {
    irt_ext_test_print("do_thread_priority_test: pthread_join failed: %s\n",
                       strerror(ret));
    return 1;
  }

  if (env->last_set_thread_nice != NICE_REALTIME) {
    irt_ext_test_print("do_thread_priority_test: Thread Priority was not"
                       " recorded in the environment.\n");
    return 1;
  }
#endif

  return 0;
}

/* Do futex tests. */
static int do_mutex_test(struct threading_environment *env) {
  /*
   * TODO(mcgrathr): Rewrite these tests not to use implementation internals.
   */
#ifndef __GLIBC__
  if (env->num_futex_wait_calls != 0 || env->num_futex_wake_calls != 0) {
    irt_ext_test_print("do_mutex_test: Threading env not initialized.\n");
    return 1;
  }

  pthread_mutex_t mutex;
  int ret = pthread_mutex_init(&mutex, NULL);
  if (0 != ret) {
    irt_ext_test_print("do_mutex_test: pthread_mutex_init failed: %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_mutex_lock(&mutex);
  if (0 != ret) {
    irt_ext_test_print("do_mutex_test: pthread_mutex_lock failed: %s\n",
                       strerror(ret));
    return 1;
  }

  /* Obtain current mutex_state and be sure we get it before thread creation. */
  const int locked_mutex_state = mutex.mutex_state;
  __sync_synchronize();

  pthread_t thread_id;
  ret = pthread_create(&thread_id, NULL, mutex_thread, &mutex);
  if (0 != ret) {
    irt_ext_test_print("do_mutex_test: pthread_create failed: %s\n",
                       strerror(ret));
    return 1;
  }

  /*
   * Unfortunately in order to test if the futex calls are being called
   * properly, we need to rely on the internals of mutex to wait for the
   * other thread to actually begin waiting on the lock. Without it, there is
   * no way to guarantee that the other thread waiting to be signalled. We are
   * using extra knowledge that mutex_state will be changed when the other
   * thread calls pthread_mutex_lock().
   */
  while (locked_mutex_state == mutex.mutex_state) {
    sched_yield();
  }

  ret = pthread_mutex_unlock(&mutex);
  if (0 != ret) {
    irt_ext_test_print("do_mutex_test: pthread_mutex_unlock failed: %s\n",
                       strerror(ret));
    return 1;
  }

  /*
   * Another unfortunate fact, we cannot use pthread_join to check if the
   * thread is done, this will trigger futex calls which will interfere
   * with our test. We will get around this by locking the mutex again to
   * make sure the mutex is at least no longer being used.
   */
  ret = pthread_mutex_lock(&mutex);
  if (0 != ret) {
    irt_ext_test_print("do_mutex_test: pthread_mutex_lock failed: %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_mutex_unlock(&mutex);
  if (0 != ret) {
    irt_ext_test_print("do_mutex_test: pthread_mutex_unlock failed: %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_mutex_destroy(&mutex);
  if (0 != ret) {
    irt_ext_test_print("do_mutex_test: pthread_mutex_destroy failed: %s\n",
                       strerror(ret));
    return 1;
  }

  /*
   * It's also unfortunate that we cannot guarantee how implementations will
   * actually call the futex commands. Even the wait call may or may not be
   * called when the mutex_state transforms (for example, as of this writing
   * our current implementation of nc_mutex.c does an extra iteration of the
   * loop checking if the mutex is waiting on anyone, this makes it likely that
   * that we would skip the futex wait call here). The only guarantee
   * is that when we are waking up the other thread, the main thread
   * will at least "attempt" to wake up the other thread, so we can test for
   * at least 1 wake call.
   */
  if (env->num_futex_wake_calls == 0) {
    irt_ext_test_print("do_mutex_test: pthread_mutex_trylock() futex calls not"
                       " properly intercepted. Expected at least 1 wake"
                       " call.\n");
    return 1;
  }

  void *thread_status = NULL;
  ret = pthread_join(thread_id, &thread_status);
  if (0 != ret) {
    irt_ext_test_print("do_mutex_test: pthread_join failed: %s\n",
                       strerror(ret));
    return 1;
  }

  ret = (int) ((uintptr_t) thread_status);
  if (0 != ret) {
    irt_ext_test_print("do_mutex_test: mutex lock thread failed: %s\n",
                       strerror(ret));
    return 1;
  }
#endif

  return 0;
}

static int do_semaphore_test(struct threading_environment *env) {
  /*
   * TODO(mcgrathr): Rewrite these tests not to use implementation internals.
   */
#ifndef __GLIBC__
  if (env->num_futex_wait_calls != 0 || env->num_futex_wake_calls != 0) {
    irt_ext_test_print("do_semaphore_test: Threading env not initialized.\n");
    return 1;
  }

  sem_t semaphore;
  if (sem_init(&semaphore, 0, 0) != 0) {
    irt_ext_test_print("do_semaphore_test: sem_init failed: %s\n",
                       strerror(errno));
    return 1;
  }

  struct semaphore_thread_args thread_args = {
    0,
    &semaphore,
  };

  pthread_t thread_id;
  int ret = pthread_create(&thread_id, NULL, semaphore_thread, &thread_args);
  if (0 != ret) {
    irt_ext_test_print("do_semaphore_test: pthread_create failed: %s\n",
                       strerror(ret));
    return 1;
  }

  /*
   * At this point we need to spin until we know that semaphore_thread has
   * begun waiting on the semaphore. Most semaphore's simply set the count
   * value to be negative to signify that there are threads waiting, but
   * according to the specification it is valid for the count to return
   * zero as well. Unfortunately if the count cannot be relied upon we have
   * to look at the internals of sem_t which is implementation specific.
   */
  /*
   * TODO(dyen): Currently we are only testing newlib, add in macros to
   * test using "if (sem_getvalue() < 0)" for other libraries.
   */
  while (semaphore.nwaiters == 0) {
    sched_yield();
  }

  if (sem_post(&semaphore) != 0) {
    irt_ext_test_print("do_semaphore_test: sem_post failed: %s\n",
                       strerror(errno));
    return 1;
  }

  /*
   * The environment checks must be done before the thread is joined because
   * thread joins will trigger their own futex calls. We also cannot check if
   * the wait call has been called until after sem_post() and that the thread
   * state is finished. This is because it is still possible to increment
   * the semaphore counter before the semaphore has truly begun waiting.
   * We also cannot guarantee that a wake is actually triggered, because
   * the waiter thread could have been triggered to wake up between setting
   * the number of waiters variable and actually waiting.
   */
  while (!thread_args.finished) {
    sched_yield();
  }

  if (env->num_futex_wait_calls == 0) {
    irt_ext_test_print("do_semaphore_test: futex wait call not triggered.\n");
    return 1;
  }

  void *thread_status = NULL;
  ret = pthread_join(thread_id, &thread_status);
  if (0 != ret) {
    irt_ext_test_print("do_semaphore_test: pthread_join failed: %s\n",
                       strerror(ret));
    return 1;
  }

  if (sem_destroy(&semaphore) != 0) {
    irt_ext_test_print("do_semaphore_test: sem_destroy failed: %s\n",
                       strerror(errno));
    return 1;
  }

  ret = (int) ((uintptr_t) thread_status);
  if (0 != ret) {
    irt_ext_test_print("do_semaphore_test: mutex lock thread failed: %s\n",
                       strerror(ret));
    return 1;
  }
#endif

  return 0;
}

static int do_cond_signal_test(struct threading_environment *env) {
  /*
   * TODO(mcgrathr): Rewrite these tests not to use implementation internals.
   */
#ifndef __GLIBC__
  if (env->num_futex_wake_calls != 0) {
    irt_ext_test_print("do_cond_signal_test: Threading env not initialized.\n");
    return 1;
  }

  pthread_cond_t cond;
  int ret = pthread_cond_init(&cond, NULL);
  if (0 != ret) {
    irt_ext_test_print("do_cond_signal_test: pthread_cond_init failed: %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_cond_signal(&cond);
  if (0 != ret) {
    irt_ext_test_print("do_cond_signal_test: pthread_cond_signal failed: %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_cond_destroy(&cond);
  if (0 != ret) {
    irt_ext_test_print("do_cond_signal_test: pthread_cond_destroy failed: %s\n",
                       strerror(ret));
    return 1;
  }

  if (env->num_futex_wake_calls != 1) {
    irt_ext_test_print("do_cond_signal_test: did not trigger wake:\n"
                       "  Expected 1 Futex Wake call.\n"
                       "  Triggered %d Futex Wake calls.\n",
                       env->num_futex_wake_calls);
    return 1;
  }
#endif

  return 0;
}

static int do_cond_broadcast_test(struct threading_environment *env) {
  /*
   * TODO(mcgrathr): Rewrite these tests not to use implementation internals.
   */
#ifndef __GLIBC__
  if (env->num_futex_wake_calls != 0) {
    irt_ext_test_print("do_cond_broadcast_test: Env not initialized.\n");
    return 1;
  }

  pthread_cond_t cond;
  int ret = pthread_cond_init(&cond, NULL);
  if (0 != ret) {
    irt_ext_test_print("do_cond_broadcast_test: pthread_cond_init fail: %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_cond_broadcast(&cond);
  if (0 != ret) {
    irt_ext_test_print("do_cond_broadcast_test: pthread_cond_signal failed:"
                       " %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_cond_destroy(&cond);
  if (0 != ret) {
    irt_ext_test_print("do_cond_broadcast_test: pthread_cond_destroy failed:"
                       " %s\n",
                       strerror(ret));
    return 1;
  }

  if (env->num_futex_wake_calls != 1) {
    irt_ext_test_print("do_cond_broadcast_test: did not trigger wake:\n"
                       "  Expected 1 Futex Wake call.\n"
                       "  Triggered %d Futex Wake calls.\n",
                       env->num_futex_wake_calls);
    return 1;
  }
#endif

  return 0;
}

static int do_cond_timed_wait_test(struct threading_environment *env) {
  /*
   * TODO(mcgrathr): Rewrite these tests not to use implementation internals.
   */
#ifndef __GLIBC__
  if (env->num_futex_wait_calls != 0) {
    irt_ext_test_print("do_cond_timed_wait_test: Env not initialized.\n");
    return 1;
  }

  pthread_mutex_t mutex;
  int ret = pthread_mutex_init(&mutex, NULL);
  if (0 != ret) {
    irt_ext_test_print("do_cond_timed_wait_test: pthread_mutex_init failed:"
                       " %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_mutex_lock(&mutex);
  if (0 != ret) {
    irt_ext_test_print("do_cond_timed_wait_test: pthread_mutex_lock failed:"
                       " %s\n",
                       strerror(ret));
    return 1;
  }

  pthread_cond_t cond;
  ret = pthread_cond_init(&cond, NULL);
  if (0 != ret) {
    irt_ext_test_print("do_cond_timed_wait_test: pthread_cond_init failed:"
                       " %s\n",
                       strerror(ret));
    return 1;
  }

  struct timespec wait_time = { TEST_TIME_VALUE + TEST_WAIT_TIME };
  env->current_time = TEST_TIME_VALUE;
  ret = pthread_cond_timedwait(&cond, &mutex, &wait_time);
  if (ETIMEDOUT != ret) {
    irt_ext_test_print("do_cond_timed_wait_test: pthread_cond_timedwait_abs"
                       " expected to time out: %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_cond_destroy(&cond);
  if (0 != ret) {
    irt_ext_test_print("do_cond_timed_wait_test: pthread_cond_destroy failed:"
                       " %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_mutex_unlock(&mutex);
  if (0 != ret) {
    irt_ext_test_print("do_cond_timed_wait_test: pthread_mutex_unlock failed:"
                       " %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_mutex_destroy(&mutex);
  if (0 != ret) {
    irt_ext_test_print("do_cond_timed_wait_test: pthread_mutex_destroy failed:"
                       " %s\n",
                       strerror(ret));
    return 1;
  }

  if (env->num_futex_wait_calls != 1) {
    irt_ext_test_print("do_cond_timed_wait_test: did not trigger wait:\n"
                       "  Expected 1 Futex Wait call.\n"
                       "  Triggered %d Futex Wait calls.\n",
                       env->num_futex_wait_calls);
    return 1;
  }

  if (env->current_time != (TEST_TIME_VALUE + TEST_WAIT_TIME)) {
    irt_ext_test_print("do_cond_timed_wait_test: did not wait for"
                       " expected duration.\n"
                       "  Expected duration: %d.\n"
                       "  Waited duration: %d.\n",
                       TEST_WAIT_TIME,
                       env->current_time - TEST_TIME_VALUE);
    return 1;
  }
#endif

  return 0;
}

static int do_cond_wait_test(struct threading_environment *env) {
  /*
   * TODO(mcgrathr): Rewrite these tests not to use implementation internals.
   */
#ifndef __GLIBC__
  if (env->num_futex_wait_calls != 0 || env->num_futex_wake_calls != 0) {
    irt_ext_test_print("do_cond_wait_test: Env not initialized.\n");
    return 1;
  }

  pthread_mutex_t mutex;
  int ret = pthread_mutex_init(&mutex, NULL);
  if (0 != ret) {
    irt_ext_test_print("do_cond_wait_test: pthread_mutex_init failed:"
                       " %s\n",
                       strerror(ret));
    return 1;
  }

  pthread_cond_t cond;
  ret = pthread_cond_init(&cond, NULL);
  if (0 != ret) {
    irt_ext_test_print("do_cond_wait_test: pthread_cond_init failed:"
                       " %s\n",
                       strerror(ret));
    return 1;
  }

  struct CondWaitThreadArg arg = {
    CondWaitStateInitialized,
    &mutex,
    &cond,
  };

  pthread_t thread_id;
  ret = pthread_create(&thread_id, NULL, cond_wait_thread, &arg);
  if (0 != ret) {
    irt_ext_test_print("do_cond_wait_test: pthread_create failed: %s\n",
                       strerror(ret));
    return 1;
  }

  /* Wait for cond_wait_thread to be ready. */
  while (arg.state != CondWaitStateReady) {
    sched_yield();
  }

  /* Indicate that we will be signally now. */
  arg.state = CondWaitStateSignalling;

  /* Continually signal the condition until the state is signalled. */
  while (arg.state != CondWaitStateSignalled) {
    pthread_cond_signal(&cond);
    sched_yield();
  }

  /*
   * Because joining threads also calls futex calls, we must test the
   * thread environment before we join the thread.
   */
  if (env->num_futex_wake_calls == 0) {
    irt_ext_test_print("do_cond_wait_test: did not trigger wake calls:\n"
                       "  Expected at least 1 Futex Wake call.\n"
                       "  Triggered 0 Futex Wake calls.\n");
    return 1;
  }

  if (env->num_futex_wait_calls != 1) {
    irt_ext_test_print("do_cond_wait_test: did not trigger wait:\n"
                       "  Expected 1 Futex Wait call.\n"
                       "  Triggered %d Futex Wait calls.\n",
                       env->num_futex_wait_calls);
    return 1;
  }

  void *thread_status = NULL;
  ret = pthread_join(thread_id, &thread_status);
  if (0 != ret) {
    irt_ext_test_print("do_cond_wait_test: pthread_join failed: %s\n",
                       strerror(ret));
    return 1;
  }

  ret = (int) ((uintptr_t) thread_status);
  if (0 != ret) {
    irt_ext_test_print("do_cond_wait_test: mutex lock thread failed: %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_cond_destroy(&cond);
  if (0 != ret) {
    irt_ext_test_print("do_cond_wait_test: pthread_cond_destroy failed:"
                       " %s\n",
                       strerror(ret));
    return 1;
  }

  ret = pthread_mutex_destroy(&mutex);
  if (0 != ret) {
    irt_ext_test_print("do_cond_wait_test: pthread_mutex_destroy failed:"
                       " %s\n",
                       strerror(ret));
    return 1;
  }
#endif

  return 0;
}

static const TYPE_thread_test g_thread_tests[] = {
  /* Basic pthread tests. */
  do_thread_creation_test,
  do_thread_priority_test,

  /* Do futex tests. */
  do_mutex_test,
  do_semaphore_test,
  do_cond_signal_test,
  do_cond_broadcast_test,
  do_cond_timed_wait_test,
  do_cond_wait_test,
};

static void setup(struct threading_environment *env) {
  init_threading_environment(env);
  activate_threading_env(env);
}

static void teardown(void) {
  deactivate_threading_env();
}

DEFINE_TEST(Thread, g_thread_tests, struct threading_environment,
            setup, teardown)
