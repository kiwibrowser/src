/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Testing suite for NativeClient threads
 */

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "native_client/src/untrusted/valgrind/dynamic_annotations.h"

#define ARRAY_SIZE(x) (sizeof (x) / sizeof (x)[0])
#define TIMEOUT_TIME_NS       (500 * 1000 * 1000)  /* 500 ms in ns */
#define TIMEOUT_EARLY_MS      (18)
#define TIMEOUT_CHECK_NS      (TIMEOUT_TIME_NS - TIMEOUT_EARLY_MS * 1000 * 1000)
/*
 * It appears that on Windows, sometimes the wait just returns early,
 * and greater "slop" is required -- so we permit TIMEOUT_EARLY_MS
 * number of milliseconds.
 *
 * The observed early wake up on Windows is ~15ms, which is the
 * Windows scheduling quantum.
 */

/*
 * On x86, we cannot have more than just shy of 8192 threads running
 * simultaneously.  This is a NaCl architectural limitation.
 * On ARM the limit is 4k.
 * Note that some thread related memory is never freed.
 * On ARM this is quite substantial (about 4kB).
 *
 * Due to a bug either in pthreads, or newlib, or even in the TCB,
 * g_num_test_loops cannot be much bigger than 100 without making
 * the test unreliable on multiple platforms.
 *
 * The number of rounds can be changed on the commandline.
 */

int g_num_test_loops  = 100;
int g_run_intrinsic   = 0;

/* Macros so we can use it for array dimensions in ISO C90 */
#define NUM_THREADS 10

__thread int tls_var = 5;

int g_ready = 0;

int g_errors = 0;

int g_verbose = 0;

#define PRINT(cond, mesg) do { if (cond) { \
                                 printf("%s:%d:%d: ", \
                                        __FUNCTION__, \
                                        __LINE__, \
                                        (int)pthread_self()); \
                                 printf mesg; \
                                 fflush(stdout); \
                               }\
                          } while (0)

#define PRINT_ERROR do { PRINT(1, ("Error\n")); g_errors++; } while (0)

/* TODO(adonovan): display informative errors. */
#define EXPECT_EQ(A, B) do { if ((A)!=(B)) PRINT_ERROR; } while (0)
#define EXPECT_NE(A, B) do { if ((A)==(B)) PRINT_ERROR; } while (0)
#define EXPECT_GE(A, B) do { if ((A)<(B)) PRINT_ERROR; } while (0)
#define EXPECT_LE(A, B) do { if ((A)>(B)) PRINT_ERROR; } while (0)

static void CheckSuccess(int err, const char *filename, int lineno,
                         const char *expr) {
  if (err != 0) {
    printf("pthread function failed with errno %i at %s:%i: %s\n",
           err, filename, lineno, expr);
    _exit(1);
  }
}

#define CHECK_OK(expr) (CheckSuccess((expr), __FILE__, __LINE__, #expr))


#define TEST_FUNCTION_START int local_error = g_errors; PRINT(1, ("Start\n"))

#define TEST_FUNCTION_END if (local_error == g_errors) { \
                            PRINT(1, ("OK\n")); \
                          } else { \
                            PRINT(1, ("FAILED\n")); \
                          }


struct SYNC_DATA {
  pthread_mutex_t mutex;
  pthread_cond_t cv;
};


typedef void* (*ThreadFunction)(void *state);


void* FastThread(void *userdata) {
  /* do nothing and immediately exit */
  return 0;
}

/* Dispatches to pthread_create while allowing for a large, but
 * finite number of attempts to get past EAGAIN by busylooping.
 */

int pthread_create_check_eagain(pthread_t *thread_id,
                                pthread_attr_t *attr,
                                void *(*func) (void *),
                                void *state) {
  int64_t loop_c = 0;
  int p = 0;

  while (EAGAIN == (p = pthread_create(thread_id, attr, func, state))) {
    /* Busyloop. The comparison slows things down a little. */
    /* The 6000 is an arbitrary cut-off point for the busyloop */
    EXPECT_LE(loop_c, 6000);
    loop_c++;
  }

  return p;
}

/* creates and waits via pthread_join() for thread to exit */
void CreateWithJoin(ThreadFunction func, void *state) {
  pthread_t thread_id;
  void* thread_ret;
  CHECK_OK(pthread_create_check_eagain(&thread_id, NULL, func, state));
  /* wait for thread to exit */
  CHECK_OK(pthread_join(thread_id, &thread_ret));
}


/* creates as detached thread, cannot join */
void CreateDetached(void) {
  pthread_t thread_id;
  pthread_attr_t attr;
  CHECK_OK(pthread_attr_init(&attr));
  CHECK_OK(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED));
  CHECK_OK(pthread_create_check_eagain(&thread_id, &attr, FastThread, NULL));
  /* cannot join on detached thread */
}


void* TlsThread(void* state) {
  struct SYNC_DATA* sync_data = (struct SYNC_DATA*)state;
  PRINT(g_verbose, ("start signal thread: %d\n", tls_var));
  CHECK_OK(pthread_mutex_lock(&sync_data->mutex));
  tls_var = 8;
  g_ready = 1;
  CHECK_OK(pthread_cond_signal(&sync_data->cv));
  CHECK_OK(pthread_mutex_unlock(&sync_data->mutex));
  PRINT(g_verbose, ("terminate signal thread\n"));
  return (void*)33;
}


void TestTlsAndSync(void) {
  pthread_t thread_id;
  pthread_attr_t attr;
  struct SYNC_DATA sync_data;

  TEST_FUNCTION_START;

  CHECK_OK(pthread_mutex_init(&sync_data.mutex, NULL));
  CHECK_OK(pthread_cond_init(&sync_data.cv, NULL));
  CHECK_OK(pthread_attr_init(&attr));
  CHECK_OK(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED));

  CHECK_OK(pthread_create_check_eagain(&thread_id, &attr,
                                       TlsThread, &sync_data));

  EXPECT_EQ(5, tls_var);
  CHECK_OK(pthread_mutex_lock(&sync_data.mutex));

  while (!g_ready) {
    CHECK_OK(pthread_cond_wait(&sync_data.cv, &sync_data.mutex));
  }

  EXPECT_EQ(5, tls_var);

  CHECK_OK(pthread_mutex_unlock(&sync_data.mutex));
  EXPECT_EQ(5, tls_var);
  TEST_FUNCTION_END;
}


void TestManyThreadsJoinable(void) {
  int i;
  TEST_FUNCTION_START;
  for (i = 0; i < g_num_test_loops; i++) {
    if (i % (g_num_test_loops / 10) == 0) {
      PRINT(g_verbose, ("round %d\n", i));
    }
    CreateWithJoin(FastThread, NULL);
  }
  TEST_FUNCTION_END;
}


void TestManyThreadsDetached(void) {
  int i;
  TEST_FUNCTION_START;
  for (i = 0; i < g_num_test_loops; i++) {
    if (i % (g_num_test_loops / 10) == 0) {
      PRINT(g_verbose, ("round %d\n", i));
    }
    CreateDetached();
  }
  TEST_FUNCTION_END;
}


void* SemaphoresThread(void *state) {
  sem_t* sem = (sem_t*) state;
  int i = 0, rv;
  for (i = 0; i < g_num_test_loops; i++) {
    rv = sem_wait(&sem[0]);
    EXPECT_EQ(0, rv);
    rv = sem_post(&sem[1]);
    EXPECT_EQ(0, rv);
  }
  EXPECT_EQ(g_num_test_loops, i);
  return 0;
}

void TestSemaphores(void) {
  int i;
  int rv;
  pthread_t thread_id;
  pthread_attr_t attr;
  sem_t sem[2];
  TEST_FUNCTION_START;
  sem_init(&sem[0], 0, 0);
  sem_init(&sem[1], 0, 0);

  CHECK_OK(pthread_attr_init(&attr));
  CHECK_OK(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED));

  CHECK_OK(pthread_create_check_eagain(&thread_id, &attr,
                                       SemaphoresThread, sem));

  for (i = 0; i < g_num_test_loops; i++) {
    if (i % (g_num_test_loops / 10) == 0) {
      PRINT(g_verbose, ("round %d\n", i));
    }
    rv = sem_post(&sem[0]);
    EXPECT_EQ(0, rv);
    rv = sem_wait(&sem[1]);
    EXPECT_EQ(0, rv);
  }
  sem_destroy(&sem[0]);
  sem_destroy(&sem[1]);
  TEST_FUNCTION_END;
}

void TestSemaphoreInitDestroy(void) {
  sem_t sem;
  int rv;
  TEST_FUNCTION_START;

  rv = sem_init(&sem, 0, (unsigned) SEM_VALUE_MAX + 1);
  EXPECT_EQ(-1, rv);  /* failure */

  rv = sem_init(&sem, 0, SEM_VALUE_MAX);
  EXPECT_EQ(0, rv);  /* success */
  rv  = sem_destroy(&sem);
  EXPECT_EQ(0, rv);

  rv = sem_init(&sem, 0, 0);
  EXPECT_EQ(0, rv);  /* success */
  rv  = sem_destroy(&sem);
  EXPECT_EQ(0, rv);

  TEST_FUNCTION_END;
}

void TestTryLockReturnValue(void) {
  pthread_mutex_t mutex;
  int rv;
  TEST_FUNCTION_START;

  CHECK_OK(pthread_mutex_init(&mutex, NULL));
  CHECK_OK(pthread_mutex_lock(&mutex));
  rv = pthread_mutex_trylock(&mutex);

  EXPECT_EQ(EBUSY, rv);

  TEST_FUNCTION_END;
}

void TestDoubleUnlockReturnValue(void) {
  pthread_mutex_t mutex;
  int rv;
  TEST_FUNCTION_START;

  /*
   * Calling pthread_mutex_unlock on an unlocked mutex is actually
   * undefined behavior under POSIX unless it's an ERRORCHECK mutex.
   */
  pthread_mutexattr_t attr;
  CHECK_OK(pthread_mutexattr_init(&attr));
  CHECK_OK(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));
  CHECK_OK(pthread_mutex_init(&mutex, &attr));

  CHECK_OK(pthread_mutex_lock(&mutex));
  CHECK_OK(pthread_mutex_unlock(&mutex));
  rv = pthread_mutex_unlock(&mutex);

  EXPECT_EQ(EPERM, rv);

  TEST_FUNCTION_END;
}

void TestUnlockUninitializedReturnValue(void) {
  pthread_mutex_t mutex;
  int rv;
  TEST_FUNCTION_START;

  /*
   * Calling pthread_mutex_unlock on an unlocked mutex is actually
   * undefined behavior under POSIX unless it's an ERRORCHECK mutex.
   */
  pthread_mutexattr_t attr;
  CHECK_OK(pthread_mutexattr_init(&attr));
  CHECK_OK(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));
  CHECK_OK(pthread_mutex_init(&mutex, &attr));

  rv = pthread_mutex_unlock(&mutex);

  EXPECT_EQ(EPERM, rv);

  TEST_FUNCTION_END;
}

pthread_once_t once_control = PTHREAD_ONCE_INIT;

/*
 * The nacl-newlib pthread.h declares this type, but glibc's pthread.h does not.
 */
#ifdef __GLIBC__
typedef int AtomicInt32;
#endif

void pthread_once_routine(void) {
  static AtomicInt32 count = 0;
  AtomicInt32 res =  __sync_fetch_and_add(&count, 1);
  EXPECT_LE(res, 1);
}

void* OnceThread(void *userdata) {
  CHECK_OK(pthread_once(&once_control, pthread_once_routine));
  return 0;
}


void TestPthreadOnce(void) {
  int i;
  TEST_FUNCTION_START;
  PRINT(g_verbose, ("creating %d threads\n", g_num_test_loops));
  for (i = 0; i < g_num_test_loops; i++) {
    pthread_t thread_id;
    pthread_attr_t attr;
    if (i % (g_num_test_loops / 10) == 0) {
      PRINT(g_verbose, ("round %d\n", i));
    }
    CHECK_OK(pthread_attr_init(&attr));
    CHECK_OK(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED));
    CHECK_OK(pthread_create_check_eagain(&thread_id, &attr, OnceThread, NULL));
  }
  TEST_FUNCTION_END;
}

void* RecursiveLockThread(void *state) {
  int i;
  pthread_mutex_t *lock = state;

  for (i = 0; i < g_num_test_loops; ++i) {
    CHECK_OK(pthread_mutex_lock(lock));
  }

  for (i = 0; i < g_num_test_loops; ++i) {
    CHECK_OK(pthread_mutex_unlock(lock));
  }

  return 0;
}

void TestRecursiveMutex(void) {
  pthread_mutexattr_t attr;
  pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
  pthread_t tid[NUM_THREADS];
  int i = 0;
  TEST_FUNCTION_START;

  PRINT(g_verbose, ("starting threads\n"));
  for (i = 0; i < NUM_THREADS; ++i) {
    CHECK_OK(pthread_create_check_eagain(&tid[i], NULL,
                                         RecursiveLockThread, &mutex));
  }

  PRINT(g_verbose, ("joining threads\n"));
  for (i = 0; i < NUM_THREADS; ++i) {
    CHECK_OK(pthread_join(tid[i], NULL));
  }

  PRINT(g_verbose, ("checking\n"));
  CHECK_OK(pthread_mutex_lock(&mutex));
  CHECK_OK(pthread_mutex_trylock(&mutex));
  CHECK_OK(pthread_mutex_unlock(&mutex));
  CHECK_OK(pthread_mutex_unlock(&mutex));

  CHECK_OK(pthread_mutex_destroy(&mutex));
  memset(&mutex, 0, sizeof(mutex));

  CHECK_OK(pthread_mutexattr_init(&attr));
  CHECK_OK(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP));
  CHECK_OK(pthread_mutex_init(&mutex, &attr));

  PRINT(g_verbose, ("starting threads\n"));
  for (i = 0; i < NUM_THREADS; ++i) {
    CHECK_OK(pthread_create_check_eagain(&tid[i], NULL,
                                         RecursiveLockThread, &mutex));
  }

  PRINT(g_verbose, ("joining threads\n"));
  for (i = 0; i < NUM_THREADS; ++i) {
    CHECK_OK(pthread_join(tid[i], NULL));
  }

  TEST_FUNCTION_END;
}


void TestErrorCheckingMutex(void) {
  pthread_mutexattr_t attr;
  pthread_mutex_t mutex;
  int rv;

  TEST_FUNCTION_START;
  CHECK_OK(pthread_mutexattr_init(&attr));
  CHECK_OK(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP));
  CHECK_OK(pthread_mutex_init(&mutex, &attr));

  rv = pthread_mutex_unlock(&mutex);
  EXPECT_NE(0, rv);

  CHECK_OK(pthread_mutex_lock(&mutex));

  rv = pthread_mutex_trylock(&mutex);
  EXPECT_NE(0, rv);

  CHECK_OK(pthread_mutex_unlock(&mutex));

  rv = pthread_mutex_unlock(&mutex);
  EXPECT_NE(0, rv);

  TEST_FUNCTION_END;
}


void tsd_destructor(void *arg) {
  *(int*)arg += 1;
}

pthread_key_t tsd_key;


void* TsdThread(void *state) {
  CHECK_OK(pthread_setspecific(tsd_key, state));

  return 0;
}


void TestTSD(void) {
  int rv;
  void* ptr;
  int destructor_count = 0;
  TEST_FUNCTION_START;
  CHECK_OK(pthread_key_create(&tsd_key, tsd_destructor));

  CHECK_OK(pthread_setspecific(tsd_key, &rv));

  ptr = pthread_getspecific(tsd_key);
  EXPECT_EQ(ptr, &rv);

  CreateWithJoin(TsdThread, &destructor_count);
  EXPECT_EQ(1, destructor_count);

  CHECK_OK(pthread_key_delete(tsd_key));

  TEST_FUNCTION_END;
}


void *PthreadExitThread(void *unused) {
  pthread_exit((void *) 1234);
  /* Should not reach here. */
  abort();
  return NULL;
}

void TestPthreadExit(void) {
  pthread_t tid;
  void *result;

  TEST_FUNCTION_START;
  CHECK_OK(pthread_create_check_eagain(&tid, NULL, PthreadExitThread, NULL));
  CHECK_OK(pthread_join(tid, &result));
  EXPECT_EQ(result, (void *) 1234);
  TEST_FUNCTION_END;
}


void* MallocSmallThread(void *userdata) {
  void* ptr = 0;
  int i;
  for (i = 0; i < g_num_test_loops; ++i) {
    ptr = (void*) malloc(16);
    EXPECT_NE(NULL, ptr);
  }
  return ptr;
}


void TestMallocSmall(void) {
  int i = 0;
  pthread_t tid[NUM_THREADS];
  TEST_FUNCTION_START;

  PRINT(g_verbose, ("starting threads\n"));
  for (i = 0; i < NUM_THREADS; ++i) {
    CHECK_OK(pthread_create_check_eagain(&tid[i], NULL,
                                         MallocSmallThread, NULL));
  }

  PRINT(g_verbose, ("joining threads\n"));
  for (i = 0; i < NUM_THREADS; ++i) {
    void* mem;
    CHECK_OK(pthread_join(tid[i], &mem));
    free(mem);
  }

  TEST_FUNCTION_END;
}


/* Test large allocations and deallocations in order to cover
   grow_heap() and shrink_heap() in glibc's malloc/arena.c. */
void* MallocLargeThread(void *unused) {
  void *blocks[100];
  int i;
  for (i = 0; i < 100; i++) {
    blocks[i] = malloc(0x1000);
    EXPECT_NE(blocks[i], NULL);
  }
  for (i = 0; i < 100; i++) {
    free(blocks[i]);
  }
  return NULL;
}

void TestMallocLarge(void) {
  int i = 0;
  pthread_t tid[NUM_THREADS];
  TEST_FUNCTION_START;

  for (i = 0; i < NUM_THREADS; i++) {
    CHECK_OK(pthread_create_check_eagain(&tid[i], NULL,
                                         MallocLargeThread, NULL));
  }
  for (i = 0; i < NUM_THREADS; i++) {
    CHECK_OK(pthread_join(tid[i], NULL));
  }
  TEST_FUNCTION_END;
}


void* ReallocThread(void *userdata) {
  void* ptr;
  int i;
  ptr = (void*) malloc(16);
  for (i = 0; i < g_num_test_loops; ++i) {
    ptr = (void*)realloc(ptr, 32);
    EXPECT_NE(NULL, ptr);
    ptr = (void*)realloc(ptr, 64000);
    EXPECT_NE(NULL, ptr);
    ptr = (void*)realloc(ptr, 64);
    EXPECT_NE(NULL, ptr);
    ptr = (void*)realloc(ptr, 32000);
    EXPECT_NE(NULL, ptr);
    ptr = (void*)realloc(ptr, 256);
    EXPECT_NE(NULL, ptr);
  }

  return ptr;
}


void TestRealloc(void) {
  pthread_t tid[NUM_THREADS];
  int i = 0;
  TEST_FUNCTION_START;
  for (i = 0; i < NUM_THREADS; ++i) {
    CHECK_OK(pthread_create_check_eagain(&tid[i], NULL, ReallocThread, NULL));
  }

  for (i = 0; i < NUM_THREADS; ++i) {
    CHECK_OK(pthread_join(tid[i], NULL));
  }

  TEST_FUNCTION_END;
}

/* Worker threads should spin-wait for this condition before starting work. */
static volatile int workers_begin;

/* Which intrinsic are we testing? */
static enum { COMPARE_AND_SWAP, FETCH_AND_ADD } intrinsic;

/* Perform 1 million atomic increments of the counter pointed to by
 * |data|, and checks the final result.  Uses the increment strategy
 * specified by the |intrinsic| global. */
#define ATOMIC_ITERATIONS 1000000

/*
 * Define max unfairness as less than 0.1% of true fairness.
 * NOTE:  This adds potential flakiness on very exotic architectures, but
 * we are not supporting those today.
 */
#define MAX_UNFAIRNESS (1000 * NUM_THREADS)


static void* WorkerThread(void *data) {
  volatile AtomicInt32* counter = (volatile AtomicInt32*) data;
  volatile int bogus = 0;
  static int backoff[8] = { 8, 16, 32, 64, 128, 256, 1024, 2048 };
  int success = 0;

  int ii, jj, kk;

  /* NB, gets stuck on ARM QEMU. */
  while (!workers_begin)
    ;
  ANNOTATE_HAPPENS_AFTER(&workers_begin);

  for (ii = 0; ii < ATOMIC_ITERATIONS; ++ii) {
    switch (intrinsic) {
      case COMPARE_AND_SWAP:
        /* NB, not atomic on ARM QEMU. */
        for (jj = 0; ; jj++) {
          AtomicInt32 prev = *counter;
          if (__sync_val_compare_and_swap(counter, prev, prev + 1) == prev) {

            /* Add win backoff to allow other threads to win */
            for (kk = 0; kk < backoff[success & 0x7]; kk++) bogus++;

            success++;
            break;
          }

          /* Failed, so reset number of successive swaps */
          success = 0;

          /* Add a break out condition in case "volatile" is broken or
             the atomic operation is exceedingly unfair. */
          if (jj > MAX_UNFAIRNESS) {
            printf("Stuck or exceeded unfairness.\n");
            break;
          }
        }
        break;
      case FETCH_AND_ADD:
        __sync_fetch_and_add(counter, 1);
        break;
      default:
        abort();
    }
  }
  return NULL;
}

/* Runs 10 copies of WorkerThread in parallel.  The address of a
 * shared volatile AtomicInt32 counter is passed to each thread.
 */
static void CheckAtomicityUnderConcurrency(void) {
  volatile AtomicInt32 counter = 0;
  pthread_t threads[NUM_THREADS];
  int ii;

  workers_begin = 0; /* Hold on... */
  for (ii = 0; ii < ARRAY_SIZE(threads); ++ii)
    CHECK_OK(pthread_create_check_eagain(&threads[ii], NULL, &WorkerThread,
                                         (void*) &counter));

  ANNOTATE_HAPPENS_BEFORE(&workers_begin);
  ANNOTATE_IGNORE_WRITES_BEGIN();
  workers_begin = 1; /* Thunderbirds are go! */
  ANNOTATE_IGNORE_WRITES_END();

  for (ii = 0; ii < ARRAY_SIZE(threads); ++ii)
    CHECK_OK(pthread_join(threads[ii], NULL));
  EXPECT_EQ(ATOMIC_ITERATIONS * ARRAY_SIZE(threads), counter);
}

/* Test hand-written intrinsics for ARM. */
static void TestIntrinsics(void) {
  TEST_FUNCTION_START;

  /* Test uncontended behaviour: */
  {
    /* COMPARE_AND_SWAP */
    volatile AtomicInt32 x = 123;
    EXPECT_EQ(123, __sync_val_compare_and_swap(&x, 123, 42));  /* matches */
    EXPECT_EQ(42, x);  /* => swapped */
    EXPECT_EQ(42, __sync_val_compare_and_swap(&x, 43, 9876));  /* no match */
    EXPECT_EQ(42, x);  /* => unchanged */

    /* FETCH_AND_ADD */
    x = 123;
    EXPECT_EQ(123, __sync_fetch_and_add(&x, 42));
    EXPECT_EQ(165, x);
    EXPECT_EQ(165, __sync_fetch_and_add(&x, 1));
    EXPECT_EQ(166, x);
  }

  /* Test behaviour with concurrency: */
  intrinsic = COMPARE_AND_SWAP;
  CheckAtomicityUnderConcurrency();

  intrinsic = FETCH_AND_ADD;
  CheckAtomicityUnderConcurrency();

  TEST_FUNCTION_END;
}

static void TestCondvar(void) {
  int i = 0;
  pthread_cond_t cv;
  pthread_mutex_t mu;
  struct timeval  tv;
  struct timespec ts;
  int res = 0;
  TEST_FUNCTION_START;
  CHECK_OK(pthread_mutex_init(&mu, NULL));
  CHECK_OK(pthread_cond_init(&cv, NULL));

  /* We just need the condvar to expire, so we use the current time */
  res = gettimeofday(&tv, NULL);
  EXPECT_EQ(res, 0);
  ts.tv_sec = tv.tv_sec;
  ts.tv_nsec = 0;

  CHECK_OK(pthread_mutex_lock(&mu));
  /* We try several times since the wait may return for a different reason. */
  while (i < 10) {
    res = pthread_cond_timedwait(&cv, &mu, &ts);
    if (res == ETIMEDOUT)
      break;
    i++;
  }
  EXPECT_EQ(ETIMEDOUT, res);

  CHECK_OK(pthread_mutex_unlock(&mu));

  CHECK_OK(pthread_cond_destroy(&cv));
  CHECK_OK(pthread_mutex_destroy(&mu));

  TEST_FUNCTION_END;
}

static void TestMutexAttrs(void) {
  TEST_FUNCTION_START;
  int shared = -1;
  pthread_mutex_t mutex;
  pthread_mutexattr_t attr;

  /* Verify default attribute settings */
  CHECK_OK(pthread_mutexattr_init(&attr));
  CHECK_OK(pthread_mutexattr_getpshared(&attr, &shared));
  EXPECT_EQ(PTHREAD_PROCESS_PRIVATE, shared);
  CHECK_OK(pthread_mutex_init(&mutex, &attr));
  CHECK_OK(pthread_mutex_destroy(&mutex));

  /* Verify we can set attributes to their default value. */
  CHECK_OK(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE));
  CHECK_OK(pthread_mutex_init(&mutex, &attr));
  CHECK_OK(pthread_mutex_destroy(&mutex));
  CHECK_OK(pthread_mutexattr_destroy(&attr));

  /*
   * Verify that setting attributes to unsupported values fails.
   */
  CHECK_OK(pthread_mutexattr_init(&attr));
  EXPECT_EQ(ENOTSUP,
            pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED));
  CHECK_OK(pthread_mutexattr_destroy(&attr));

  TEST_FUNCTION_END;
}

static void TestCondvarAttrs(void) {
  TEST_FUNCTION_START;
  clockid_t clock_id = -1;
  int shared = -1;
  pthread_cond_t cv;
  pthread_condattr_t attr;

  /* Verify default attribute settings */
  CHECK_OK(pthread_condattr_init(&attr));
  CHECK_OK(pthread_condattr_getclock(&attr, &clock_id));
  EXPECT_EQ(CLOCK_REALTIME, clock_id);
  CHECK_OK(pthread_condattr_getpshared(&attr, &shared));
  EXPECT_EQ(PTHREAD_PROCESS_PRIVATE, shared);

  CHECK_OK(pthread_cond_init(&cv, &attr));
  CHECK_OK(pthread_cond_destroy(&cv));

  /* Verify we can set attributes to their default value. */
  CHECK_OK(pthread_condattr_setclock(&attr, CLOCK_REALTIME));
  CHECK_OK(pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE));
  CHECK_OK(pthread_cond_init(&cv, &attr));
  CHECK_OK(pthread_cond_destroy(&cv));
  CHECK_OK(pthread_condattr_destroy(&attr));

  /*
   * Verify that setting attributes to unsupported values fails.
   */
  CHECK_OK(pthread_condattr_init(&attr));
  EXPECT_EQ(ENOTSUP, pthread_condattr_setclock(&attr, CLOCK_MONOTONIC));
  EXPECT_EQ(ENOTSUP, pthread_condattr_setpshared(&attr,
                                                 PTHREAD_PROCESS_SHARED));
  CHECK_OK(pthread_condattr_destroy(&attr));

  TEST_FUNCTION_END;
}

void AddNanosecondsToTimespec(struct timespec *time, unsigned int nanoseconds) {
  EXPECT_LE(nanoseconds, 1000000000);
  time->tv_nsec += nanoseconds;
  if (time->tv_nsec > 1000000000) {
    time->tv_nsec -= 1000000000;
    time->tv_sec += 1;
  }
}

static void TestCondvarTimeout(void) {
  int i = 0;
  pthread_cond_t cv;
  pthread_mutex_t mu;
  struct timespec t_start;
  struct timespec t_timeout;
  struct timespec t_end;
  uint64_t elapsed_ns;
  int res = 0;
  TEST_FUNCTION_START;
  CHECK_OK(pthread_mutex_init(&mu, NULL));
  CHECK_OK(pthread_cond_init(&cv, NULL));

  /*
   * The timeout value for pthread_cond_timedwait is in absolute
   * CLOCK_REALTIME time, so we use the current time and add the
   * desired elapsed time to it.
   */
  res = clock_gettime(CLOCK_REALTIME, &t_start);
  EXPECT_EQ(res, 0);
  t_timeout = t_start;
  AddNanosecondsToTimespec(&t_timeout, TIMEOUT_TIME_NS);

  CHECK_OK(pthread_mutex_lock(&mu));

  /* We try several times since the wait may return for a different reason. */
  while (i < 10) {
    res = pthread_cond_timedwait(&cv, &mu, &t_timeout);
    if (res == ETIMEDOUT)
      break;
    printf("res = %d\n", res);
    i++;
  }
  printf("res = %d, ETIMEDOUT = %d\n", res, ETIMEDOUT);
  EXPECT_EQ(ETIMEDOUT, res);

  CHECK_OK(pthread_mutex_unlock(&mu));

  res = clock_gettime(CLOCK_REALTIME, &t_end);
  EXPECT_EQ(res, 0);

  elapsed_ns = 1000 * 1000 * 1000 * (t_end.tv_sec - t_start.tv_sec) +
      (t_end.tv_nsec - t_start.tv_nsec);
  printf("Elapsed time %llu ns\n", elapsed_ns);

  EXPECT_GE(elapsed_ns, TIMEOUT_CHECK_NS);

  CHECK_OK(pthread_cond_destroy(&cv));
  CHECK_OK(pthread_mutex_destroy(&mu));

  TEST_FUNCTION_END;
}

void TestScope(void) {
  pthread_attr_t attr;
  int scope;

  TEST_FUNCTION_START;

  /* Check that the default scope is PTHREAD_SCOPE_SYSTEM */
  CHECK_OK(pthread_attr_init(&attr));
  CHECK_OK(pthread_attr_getscope(&attr, &scope));
  EXPECT_EQ(PTHREAD_SCOPE_SYSTEM, scope);

  /* Setting to PTHREAD_SCOPE_PROCESS is invalid */
  EXPECT_EQ(ENOTSUP, pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS));
  EXPECT_EQ(EINVAL, pthread_attr_setscope(&attr, 0xff));

  /* Setting to PTHREAD_SCOPE_SYSTEM should work (no-op) */
  CHECK_OK(pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM));

  TEST_FUNCTION_END;
}

void TestStackSize(void) {
  pthread_attr_t attr;
  size_t stack_size, stack_size2;

  TEST_FUNCTION_START;

  CHECK_OK(pthread_attr_init(&attr));
  CHECK_OK(pthread_attr_getstacksize(&attr, &stack_size));
  stack_size *= 2;

  CHECK_OK(pthread_attr_setstacksize(&attr, stack_size));
  CHECK_OK(pthread_attr_getstacksize(&attr, &stack_size2));

  EXPECT_EQ(stack_size, stack_size2);

  TEST_FUNCTION_END;
}

struct MutexClaimerThreadArgs {
  pthread_mutex_t *mutex;
  int should_exit;
};

void *MutexClaimerThread(void *thread_arg) {
  struct MutexClaimerThreadArgs *args = thread_arg;
  for (;;) {
    pthread_mutex_lock(args->mutex);
    int should_exit = args->should_exit;
    pthread_mutex_unlock(args->mutex);
    if (should_exit)
      break;
  }
  return NULL;
}

/*
 * This is a regression test for
 * http://code.google.com/p/nativeclient/issues/detail?id=3047
 *
 * The problem was that mutexes created with PTHREAD_MUTEX_ERRORCHECK
 * didn't unlock successfully after pthread_cond_timedwait() had
 * returned with ETIMEDOUT.  This problem occurred with NaCl's
 * newlib-based libpthread.
 */
void TestErrorcheckMutexWorksWithCondvarTimeout(void) {
  TEST_FUNCTION_START;

  pthread_mutexattr_t attrs;
  pthread_mutex_t mutex;
  pthread_cond_t condvar;
  CHECK_OK(pthread_mutexattr_init(&attrs));
  CHECK_OK(pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_ERRORCHECK));
  CHECK_OK(pthread_mutex_init(&mutex, &attrs));
  CHECK_OK(pthread_mutexattr_destroy(&attrs));
  CHECK_OK(pthread_cond_init(&condvar, NULL));
  CHECK_OK(pthread_mutex_lock(&mutex));

  struct MutexClaimerThreadArgs thread_args;
  thread_args.mutex = &mutex;
  thread_args.should_exit = 0;
  pthread_t tid;
  CHECK_OK(pthread_create(&tid, NULL, MutexClaimerThread, &thread_args));

  struct timespec timeout;
  EXPECT_EQ(clock_gettime(CLOCK_REALTIME, &timeout), 0);
  /*
   * Wait for 500 ms to give MutexClaimerThread() time to run and
   * briefly claim and release the lock, which unsets the mutex's
   * owner_thread_id.
   */
  AddNanosecondsToTimespec(&timeout, 500 * 1000);
  /*
   * The bug is that pthread_cond_timedwait() fails to update the
   * mutex's owner_thread_id to the current thread in the ETIMEDOUT
   * case.
   */
  EXPECT_EQ(pthread_cond_timedwait(&condvar, &mutex, &timeout), ETIMEDOUT);
  thread_args.should_exit = 1;
  /* The bug manifests itself by pthread_mutex_unlock() returning EPERM. */
  CHECK_OK(pthread_mutex_unlock(&mutex));

  /* Clean up. */
  CHECK_OK(pthread_join(tid, NULL));
  CHECK_OK(pthread_mutex_destroy(&mutex));
  CHECK_OK(pthread_cond_destroy(&condvar));

  TEST_FUNCTION_END;
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    g_num_test_loops = atoi(argv[1]);
  }

  if (argc > 2) {
    if (strcasecmp(argv[2], "intrinsic") == 0)
      g_run_intrinsic = 1;
  }

  TestTlsAndSync();
  TestManyThreadsJoinable();
  TestManyThreadsDetached();
  TestSemaphores();
  TestSemaphoreInitDestroy();
  TestTryLockReturnValue();
  TestDoubleUnlockReturnValue();
  TestUnlockUninitializedReturnValue();
  TestPthreadOnce();
  TestRecursiveMutex();
  TestErrorCheckingMutex();
  TestTSD();
  TestPthreadExit();
  TestMallocSmall();
  TestMallocLarge();
  TestRealloc();

  /* We have disabled this test by default since it is flaky under VMWARE. */
  if (g_run_intrinsic) TestIntrinsics();

  TestMutexAttrs();
  TestCondvar();
  TestCondvarAttrs();
  TestCondvarTimeout();
  TestStackSize();
  TestScope();
  TestErrorcheckMutexWorksWithCondvarTimeout();

  return g_errors;
}
