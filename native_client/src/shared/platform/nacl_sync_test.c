/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Exercise the NaClMutex object.
 *
 * NB: Some tests are death tests, e.g., ensuring that a NaClMutex
 * object really implements a binary semaphore instead of a recursive
 * semaphore requires intentionally deadlocking and aborting via a
 * test time out.
 */
#include <string.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/shared/platform/platform_init.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/shared/platform/nacl_time.h"

#define TIMEOUT_THREAD_STACK_SIZE (8192)
#define ON_TIMEOUT_EXIT_SUCCESS ((void *) 0)
#define ON_TIMEOUT_EXIT_FAILURE ((void *) 1)

/*
 * This test contains several subtests.  The goal is to check that
 * NaClMutex objects actually implement binary mutexes.  To do so, the
 * subtests check that:
 *
 * Lock followed by another Lock leads to a deadlock;
 *
 * TryLock followed by a Lock leads to a deadlock;
 *
 * Lock followed by a TryLock leads to an NACL_SYNC_EBUSY return; and
 *
 * TryLock followed by another TryLock leads to an NACL_SYNC_EBUSY return.
 *
 * The way that deadlock detection works is not by alarm(3) timeouts,
 * since that's not cross platform (not available in Windows).
 * Instead, we spawn a separate thread via the NaClThread abstraction
 * prior to invoking the expected-to-deadlock operation.  That thread
 * will use NaClNanosleep (which is the cross platform version of
 * nanosleep(2)) to suspend itself for a while, and then invoke
 * exit(0) to shut down the whole process, which includes getting rid
 * of the deadlocked thread.  On the other hand, if the
 * expected-to-deadlock operation didn't actually cause a deadlock,
 * the main thread immediately exits with a non-zero exit status.
 *
 * This test strategy is subject to scheduler races.  It may be the
 * case that we would get a false negative -- the test would report
 * that there is no bug, because the scheduler paused the thread that
 * invoked the expected-to-deadlock operation long enough that the
 * deadlock detection thread fires.  This is expected to be extremely
 * rare, since the timeout value (defaulting to 500ms) is much larger
 * than a single scheduling quantum on any host operating system, and
 * the likelihood that the scheduler happens to interrupt the thread
 * at precisely the wrong place ought to be low.  (We expect that the
 * largest scheduling quantum that we would encounter to be ~16.7ms or
 * 60Hz.)  In any case, a false negative will not create false alarm.
 *
 * In the case where the second lock operation is a TryLock, we do not
 * expect the TryLock to block and just verify proper behavior based
 * on the return value.  We detect regressions where the TryLock
 * blocks by spawning a timeout thread that would cause the test to
 * exit with a non-zero exit status.  In these cases, we can have a
 * false positive due to scheduler problems on an extremely heavily
 * loaded machine, but in practice this should not occur.
 */


/*
 * A second thread is spawned to cause the whole process to exit after
 * g_timeout_milliseconds.
 */
uint32_t g_timeout_milliseconds = 500;

/*
 * TimeOutThread is responsible for doing deadlock detection.  If the
 * main thread hits a deadlock, then this thread will time out and
 * exit with the status specified by the thread_state argument.
 *
 * If the caller expect a deadlock in the main thread, the
 * thread_state value will be 0 for success to indicate that the
 * expected deadlock occurred; if caller does not expect a deadlock in
 * the main thread, the thread_state value should be non-zero.
 */
void WINAPI TimeOutThread(void *thread_state) {
  struct nacl_abi_timespec ts;
  int time_out_exit_status = (int) (uintptr_t) thread_state;

  ts.tv_sec = g_timeout_milliseconds / 1000;
  ts.tv_nsec = (g_timeout_milliseconds % 1000) * 1000 * 1000;

  while (0 != NaClNanosleep(&ts, &ts)) {
    /*
     * On POSIX operating systems nanosleep (which underlies
     * NaClNanosleep) can return early with errno equal to
     * NACL_ABI_EINTR if, for example, a signal interrupted its sleep.
     * When this occurs, the second timespec argument (if non-NULL) is
     * overwritten with the remaining time to sleep, and so we can
     * immediately sleep again.  We also expose the EINVAL and EFAULT
     * error returns (translated to NACL_ABI_), but since we set the
     * initial timespec value those should never occur.
     */
    continue;
  }
  /*
   * If we reach here, we assume that the main thread has deadlocked
   * and so we optimistically report that via the exit status.
   */
  exit(time_out_exit_status);
}

/*
 * The following tests do not Dtor the NaClMutex objects, since we
 * only run one test before exiting.  This is a hard requirement for
 * deadlock detection where the second lock is a NaClXMutexLock (the
 * trylock case could be simpler, though we also spawn a time-out
 * thread in that case in case the error is a deadlock rather than an
 * immediate return with NACL_SYNC_BUSY).
 */

int TestLockLock(void) {
  struct NaClMutex mu;
  struct NaClThread nt;
  printf("TestLockLock\n");
  printf("Constructing mutex\n");
  if (!NaClMutexCtor(&mu)) return 1;
  printf("Locking mutex\n");
  NaClXMutexLock(&mu);
  printf("Spawning timeout thread\n");
  /* NULL is exit status if timeout occurs */
  if (!NaClThreadCtor(&nt, TimeOutThread,
                      ON_TIMEOUT_EXIT_SUCCESS, TIMEOUT_THREAD_STACK_SIZE)) {
    return 1;
  }
  printf("Locking mutex again\n");
  NaClXMutexLock(&mu);
  /* should deadlock and timeout thread should exit process */
  printf("ERROR: Double locking succeeded?!?\n");
  return 1;
}

int TestLockTrylock(void) {
  struct NaClMutex mu;
  struct NaClThread nt;
  printf("TestLockTrylock\n");
  printf("Constructing mutex\n");
  if (!NaClMutexCtor(&mu)) return 1;
  printf("Locking mutex\n");
  NaClXMutexLock(&mu);
  printf("Spawning timeout thread\n");
  /* 1 is exit status if timeout occurs */
  if (!NaClThreadCtor(&nt, TimeOutThread,
                      ON_TIMEOUT_EXIT_FAILURE, TIMEOUT_THREAD_STACK_SIZE)) {
    return 1;
  }
  printf("Trylocking mutex\n");
  /*
   * Here the NaClMutexTryLock should not block, and the role of the
   * timeout thread is to detect implementation flaws where a trylock
   * blocks.
   */
  if (NaClMutexTryLock(&mu) == NACL_SYNC_BUSY) {
    printf("OK: trylock failed\n");
    return 0;
  }
  printf("ERROR: Trylock succeeded?!?\n");
  return 1;
}

int TestTrylockLock(void) {
  struct NaClMutex mu;
  struct NaClThread nt;
  printf("TestLockTrylock\n");
  printf("Constructing mutex\n");
  if (!NaClMutexCtor(&mu)) return 1;
  printf("Trylocking mutex\n");
  if (NaClMutexTryLock(&mu) != NACL_SYNC_OK) {
    printf("ERROR: Trylock failed\n");
    return 1;
  }
  printf("Spawning timeout thread\n");
  if (!NaClThreadCtor(&nt, TimeOutThread,
                      ON_TIMEOUT_EXIT_SUCCESS, TIMEOUT_THREAD_STACK_SIZE)) {
    return 1;
  }
  printf("Locking mutex\n");
  NaClXMutexLock(&mu);
  printf("ERROR: Lock succeeded?!?\n");
  return 1;
}

int TestTrylockTrylock(void) {
  struct NaClMutex mu;
  struct NaClThread nt;
  printf("TestLockTrylock\n");
  printf("Constructing mutex\n");
  if (!NaClMutexCtor(&mu)) return 1;
  printf("Trylocking mutex\n");
  if (NaClMutexTryLock(&mu) != NACL_SYNC_OK) {
    printf("ERROR: Trylock failed\n");
    return 1;
  }
  printf("Spawning timeout thread\n");
  if (!NaClThreadCtor(&nt, TimeOutThread,
                      ON_TIMEOUT_EXIT_FAILURE, TIMEOUT_THREAD_STACK_SIZE)) {
    return 1;
  }
  printf("Trylocking mutex again\n");
  if (NaClMutexTryLock(&mu) == NACL_SYNC_BUSY) {
    printf("OK: trylock failed\n");
    return 0;
  }
  printf("ERROR: Trylock succeeded?!?\n");
  return 1;
}

struct Tests {
  char const *name;
  int (*test)(void);
};

struct Tests const tests[] = {
  { "lock_lock", TestLockLock, },
  { "lock_trylock", TestLockTrylock, },
  { "trylock_lock", TestTrylockLock, },
  { "trylock_trylock", TestTrylockTrylock, },
};

void usage(void) {
  size_t ix;

  fprintf(stderr,
          "Usage: nacl_sync_test [-t timeout_milliseconds] [-T test]\n"
          "       where <test> is one of\n");
  for (ix = 0; ix < NACL_ARRAY_SIZE(tests); ++ix) {
    fprintf(stderr, "         %s\n", tests[ix].name);
  }
}

int main(int ac, char **av) {
  int opt;
  int (*test_fn)(void) = NULL;
  size_t ix;
  int retcode;

  while (-1 != (opt = getopt(ac, av, "t:T:"))) {
    switch (opt) {
      case 't':
        g_timeout_milliseconds = strtoul(optarg, (char **) NULL, 0);
        break;
      case 'T':
        for (ix = 0; ix < NACL_ARRAY_SIZE(tests); ++ix) {
          if (!strcmp(optarg, tests[ix].name)) {
            test_fn = tests[ix].test;
            break;
          }
        }
        break;
      default:
        usage();
        return 1;
    }
  }
  if (test_fn == NULL) {
    fprintf(stderr, "No test specified\n");
    usage();
    return 1;
  }
  NaClPlatformInit();
  retcode = (*test_fn)();
  NaClPlatformFini();
  return retcode;
}
