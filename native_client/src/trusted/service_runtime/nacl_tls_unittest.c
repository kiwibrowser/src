/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
 * A stand-alone application that tests thread local storage (TLS) on the
 * current platform.  It is written to avoid locks and condition variables.
 * Since it is testing thread-specific stuff, it's good to use as little
 * *other* thread-specific stuff as possible for these tests.
 *
 * Note that this test hangs in pthread_join() on ARM QEMU.
 */

#include "native_client/src/include/build_config.h"
#if NACL_LINUX || NACL_OSX
#include <pthread.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_time.h"

/* Constants. */
static const int NSEC_PER_MSEC = 1000 * 1000;

/* A regular global variable, shared among all threads. */
volatile int g_running_thread_count;

/*
 * The TLS to be tested.
 * This variable is used in only one function. Since we're testing that the
 * memory is actually different for each thread, we need to make sure the
 * compiler does not optimize out the store/load of this variable or
 * rearrange when the store happens.  As such, make the TLS variable
 * "volatile".
 */
THREAD volatile int tls_data;

/* Parameter block used when starting a thread. */
struct ThreadData {
  int thread_num;
  int total_num_threads;
  int *p_tls_data_return;
};

/* Initializes NaCl modules. */
static void Init(void) {
  const char *nacl_verbosity = getenv("NACLVERBOSITY");

  NaClLogModuleInit();
  NaClLogSetVerbosity((NULL == nacl_verbosity)
                      ? 0
                      : strtol(nacl_verbosity, (char **) 0, 0));
  NaClTimeInit();
}

/* Shuts down NaCl modules. */
static void Fini(void) {
  NaClTimeFini();
  NaClLogModuleFini();
}

/* Prints an error message and exits with "failure" status. */
static void ErrorExit(void) {
  NaClLog(LOG_ERROR, "TEST FAILED\n");
  Fini();
  exit(1);
}

/*
 * Waits for g_running_thread_count to exactly equal final_thread_count.
 * Returns true if final_thread_count reached, false if timed out.
 */
static int WaitForThreads(int final_thread_count) {
  const int SLEEP_NSEC = 10 * NSEC_PER_MSEC; /* 10ms */
  const int SLEEP_ITERATIONS = 1000;         /* *10ms = 10 sec */

  int i;
  struct nacl_abi_timespec sleep_time;

  sleep_time.tv_sec = 0;
  sleep_time.tv_nsec = SLEEP_NSEC;

  for (i = 0; i < SLEEP_ITERATIONS; ++i) {
    if (final_thread_count == g_running_thread_count) {
      return 1;
    }
    NaClNanosleep(&sleep_time, NULL);
  }
  return 0;
}

/* Executes the test of TLS for each thread. */
#if NACL_LINUX || NACL_OSX
typedef void *thread_param_t;
typedef void *thread_return_t;
#define OS_API_TYPE
#elif NACL_WINDOWS
typedef LPVOID thread_param_t;
typedef DWORD thread_return_t;
#define OS_API_TYPE WINAPI
#endif
static thread_return_t OS_API_TYPE ThreadEntryPoint(thread_param_t p) {
  struct ThreadData *param = (struct ThreadData *) p;
  int my_thread_num = param->thread_num;

  /* Set our TLS to our expected value. */
  tls_data = my_thread_num * 2;

  /* Tell the main thread that we are running. */
  ++g_running_thread_count;

  /* Wait for all threads to be running. */
  WaitForThreads(param->total_num_threads);

  /*
   * All of the threads have started and have written to their own TLS.
   * Read our TLS and give it back to the main thread.
   */
  param->p_tls_data_return[my_thread_num] = tls_data;

  return 0;
}

#if NACL_LINUX || NACL_OSX
/*
 * Creates a thread on Linux.
 * Returns 0 for success.
 */
static int MyCreateThread(pthread_t *p_pthread, void *p) {
  return pthread_create(p_pthread, NULL, ThreadEntryPoint, p);
}

/*
 * Waits for a thread to finish on Linux.
 * Returns 0 for success.
 */
static int MyWaitForThreadExit(pthread_t pthread) {
  return pthread_join(pthread, NULL);
}

#elif NACL_WINDOWS
/*
 * Creates a thread on Windows.
 * Returns 0 for success.
 */
static int MyCreateThread(HANDLE *p_handle, void *p) {
  *p_handle = CreateThread(NULL,   /* security. */
                           0,      /* <64K stack size. */
                           ThreadEntryPoint,
                           p,      /* Thread data. */
                           0,      /* Thread runs immediately. */
                           NULL);  /* OS version of thread id. */
  return (NULL != *p_handle) ? 0 : 1;
}

/*
 * Waits for a thread to finish on Windows.
 * Returns 0 for success.
 */
static int MyWaitForThreadExit(HANDLE handle) {
  static const DWORD MAX_WAIT_MSEC = 5000;

  DWORD result = WaitForMultipleObjects(1,              /* Thread count. */
                                        &handle,        /* Thread handle. */
                                        TRUE,           /* Wait for all. */
                                        MAX_WAIT_MSEC); /* Max wait time */
  return (WAIT_OBJECT_0 == result) ? 0 : 1;
}
#endif

/* Tests that threads can access TLS. */
int main(void) {
#define NUM_THREADS (10)

  int i;
  int rc;
#if NACL_LINUX || NACL_OSX
  pthread_t thread_id[NUM_THREADS];
#elif NACL_WINDOWS
  HANDLE thread_id[NUM_THREADS];
#endif
  int tls_data_return[NUM_THREADS];
  struct ThreadData param;
  int test_passed;

  /* Initialize NaCl. */
  Init();

  /* Initialize global variables. */
  g_running_thread_count = 0;

  /* Initialize the per-thread data. */
  memset(thread_id, 0x00, sizeof(thread_id));
  memset(tls_data_return, 0xff, sizeof(tls_data_return));

  /* Initialize the thread parameter block. */
  param.total_num_threads = NUM_THREADS;
  param.p_tls_data_return = tls_data_return;

  /* Start each thread and have it set its TLS. */
  for (i = 0; i < NUM_THREADS; ++i) {
    param.thread_num = i;
    rc = MyCreateThread(&thread_id[i], &param);
    if (0 != rc) {
      NaClLog(LOG_ERROR, "ERROR: Could not create thread %d, rc=%d.\n", i, rc);
      ErrorExit();
    }
    if (!WaitForThreads(i + 1)) {
      NaClLog(LOG_ERROR, "ERROR: Timed out waiting for thread %d.\n", i);
      ErrorExit();
    }
  }

  /*
   * All threads have started and have written to their TLS.  As soon as the
   * last thread was ready, all of the threads should have continued
   * executing and exited.  Wait for each thread to exit, then validate its
   * TLS data.
   */
  test_passed = 1;
  for (i = 0; i < NUM_THREADS; ++i) {
    int expected = i * 2;

    /*
     * Print the expected and actual data to the console. Don't abort
     * immediately on error. This allows each thread's TLS to be examined so
     * that all of the bad data gets printed to help show a failure pattern.
     */
    rc = MyWaitForThreadExit(thread_id[i]);
    if (0 != rc) {
      NaClLog(LOG_ERROR, "ERROR: Thread %d did not exit, rc=%d.\n", i, rc);
      test_passed = 0;
    } else  if (expected != tls_data_return[i]) {
      NaClLog(LOG_ERROR, "ERROR: Thread %d TLS data, expected=%d, actual=%d.\n",
               i, expected, tls_data_return[i]);
      test_passed = 0;
    } else {
      NaClLog(LOG_INFO, "OK: Thread %d TLS data, expected=%d, actual=%d.\n",
               i, expected, tls_data_return[i]);
    }
  }
  if (!test_passed) {
    ErrorExit();
  }

  NaClLog(LOG_INFO, "TEST PASSED\n");
  Fini();
  return 0;
}
