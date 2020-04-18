/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "native_client/src/include/nacl_macros.h"

/*
 * This test resembles the trusted version which can be found in
 * src/shared/platform/nacl_clock_test.c, but uses the stable ABI
 * to test the integration correctness. When making changes, please
 * keep both tests in sync if possible.
 */

/*
 * On an unloaded i7, 1ms with a 1.25 fuzziness factor and a 100,000
 * ns constant syscall overhead works fine.  On bots, we have to be
 * much more generous.  (This is especially true for qemu-based
 * testing.)
 */
#define DEFAULT_NANOSLEEP_EXTRA_OVERHEAD  (10 * NACL_NANOS_PER_MILLI)
#define DEFAULT_NANOSLEEP_EXTRA_FACTOR    (100.0)
#define DEFAULT_NANOSLEEP_TIME            (10 * NACL_NANOS_PER_MILLI)

#define THREAD_CYCLES     100000000

/*
 * Global testing parameters -- fuzziness coefficients in determining
 * what is considered accurate.
 */
static int      g_cputime = 0;
static double   g_fuzzy_factor = DEFAULT_NANOSLEEP_EXTRA_FACTOR;
static uint64_t g_syscall_overhead = DEFAULT_NANOSLEEP_EXTRA_OVERHEAD;
static uint64_t g_slop_ms = 0;

/*
 * ClockMonotonicAccuracyTest samples the CLOCK_MONOTONIC
 * clock before and after invoking NaClNanosleep and computes the time
 * delta.  The test is considered to pass if the time delta is close
 * to the requested value.  "Close" is a per-host-OS attribute, thus
 * the above testing parameters.
 */
static int ClockMonotonicAccuracyTest(uint64_t sleep_nanos) {
  int             num_failures = 0;

  struct timespec t_start;
  struct timespec t_sleep;
  struct timespec t_end;

  uint64_t        elapsed_nanos;
  uint64_t        elapsed_lower_bound;
  uint64_t        elapsed_upper_bound;

  t_sleep.tv_sec  = sleep_nanos / NACL_NANOS_PER_UNIT;
  t_sleep.tv_nsec = sleep_nanos % NACL_NANOS_PER_UNIT;

  printf("\nCLOCK_MONOTONIC accuracy test:\n");

  if (0 != clock_gettime(CLOCK_MONOTONIC, &t_start)) {
    fprintf(stderr, "clock_test: clock_gettime (start) failed, error %d\n",
            errno);
    ++num_failures;
    goto done;
  }
  for (;;) {
    if (0 == nanosleep(&t_sleep, &t_sleep)) {
      break;
    }
    if (EINTR == errno) {
      /* interrupted syscall: sleep some more */
      continue;
    }
    fprintf(stderr, "clock_test: nanosleep failed, error %d\n",
            errno);
    num_failures++;
    goto done;
  }
  if (0 != clock_gettime(CLOCK_MONOTONIC, &t_end)) {
    fprintf(stderr, "clock_test: clock_gettime (end) failed, error %d\n",
            errno);
    return 1;
  }

  elapsed_nanos = (t_end.tv_sec - t_start.tv_sec) * NACL_NANOS_PER_UNIT +
      (t_end.tv_nsec - t_start.tv_nsec) + g_slop_ms * NACL_NANOS_PER_MILLI;

  elapsed_lower_bound = sleep_nanos;
  elapsed_upper_bound = (uint64_t) (sleep_nanos * g_fuzzy_factor +
                                    g_syscall_overhead);

  printf("requested sleep:   %20"PRIu64" nS\n", sleep_nanos);
  printf("elapsed sleep:     %20"PRIu64" nS\n", elapsed_nanos);
  printf("sleep lower bound: %20"PRIu64" nS\n", elapsed_lower_bound);
  printf("sleep upper bound: %20"PRIu64" nS\n", elapsed_upper_bound);

  if (elapsed_nanos < elapsed_lower_bound ||
      elapsed_upper_bound < elapsed_nanos) {
    printf("discrepancy too large\n");
    num_failures++;
  }
 done:
  printf((0 == num_failures) ? "PASSED\n" : "FAILED\n");
  return num_failures;
}

/*
 * ClockRealtimeAccuracyTest compares the time returned by
 * CLOCK_REALTIME against that returned by gettimeofday.
 */
static int ClockRealtimeAccuracyTest(void) {
  int             num_failures = 0;

  struct timespec t_now_ts;
  struct timeval  t_now_tv;

  uint64_t        t_now_ts_nanos;
  uint64_t        t_now_tv_nanos;
  int64_t         t_now_diff_nanos;

  printf("\nCLOCK_REALTIME accuracy test:\n");

  if (0 != clock_gettime(CLOCK_REALTIME, &t_now_ts)) {
    fprintf(stderr, "clock_test: clock_gettime (now) failed, error %d\n",
            errno);
    num_failures++;
    goto done;
  }
  if (0 != gettimeofday(&t_now_tv, NULL)) {
    fprintf(stderr, "clock_test: gettimeofday (now) failed, error %d\n",
            errno);
    num_failures++;
    goto done;
  }

  t_now_ts_nanos = t_now_ts.tv_sec * NACL_NANOS_PER_UNIT + t_now_ts.tv_nsec;
  t_now_tv_nanos = t_now_tv.tv_sec * NACL_NANOS_PER_UNIT +
      t_now_tv.tv_usec * NACL_NANOS_PER_MICRO;

  printf("clock_gettime:   %20"PRIu64" nS\n", t_now_ts_nanos);
  printf("gettimeofday:    %20"PRIu64" nS\n", t_now_tv_nanos);

  t_now_diff_nanos = t_now_ts_nanos - t_now_tv_nanos;
  if (t_now_diff_nanos < 0) {
    t_now_diff_nanos = -t_now_diff_nanos;
  }
  printf("time difference: %20"PRId64" nS\n", t_now_diff_nanos);

  if (t_now_ts_nanos < g_syscall_overhead) {
    printf("discrepancy too large\n");
    num_failures++;
  }
 done:
  printf((0 == num_failures) ? "PASSED\n" : "FAILED\n");
  return num_failures;
}

struct ThreadInfo {
  size_t          cycles;
  struct timespec thread_time;
  struct timespec process_time;
  int             num_failures;
};

void *ThreadFunction(void *ptr) {
  size_t            i;
  struct ThreadInfo *info = (struct ThreadInfo *) ptr;

  for (i = 1; i < info->cycles; i++) {
    /*
     * Use a barrier to ensure that the compiler does not optimize the
     * empty loop out as we really only want to burn the thread time.
     */
    __sync_synchronize();
  }

  if (0 != clock_gettime(CLOCK_THREAD_CPUTIME_ID, &info->thread_time)) {
    fprintf(stderr, "clock_test: clock_gettime (now) failed, error %d\n",
            errno);
    info->num_failures++;
    return NULL;
  }

  if (0 != clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &info->process_time)) {
    fprintf(stderr, "clock_test: clock_gettime (now) failed, error %d\n",
            errno);
    info->num_failures++;
    return NULL;
  }

  return NULL;
}

static int ClockCpuTimeAccuracyTest(void) {
  int               num_failures = 0;

  int               err;
  struct timespec   t_process_start;
  struct timespec   t_process_end;
  struct timespec   t_thread_start;
  struct timespec   t_thread_end;

  uint64_t          thread_elapsed = 0;
  uint64_t          process_elapsed = 0;
  uint64_t          child_thread_elapsed = 0;
  uint64_t          elapsed_lower_bound;
  uint64_t          elapsed_upper_bound;

  size_t            i;
  struct ThreadInfo info[10];
  pthread_t         thread[10];

  printf("\nCLOCK_PROCESS/THREAD_CPUTIME_ID accuracy test:\n");

  if (0 != clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t_thread_start)) {
    fprintf(stderr, "clock_test: clock_gettime (now) failed, error %d\n",
            errno);
    num_failures++;
    goto done;
  }

  if (0 != clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t_process_start)) {
    fprintf(stderr, "clock_test: clock_gettime (now) failed, error %d\n",
            errno);
    num_failures++;
    goto done;
  }

  for (i = 0; i < NACL_ARRAY_SIZE(thread); i++) {
    memset(&info[i], 0, sizeof info[i]);
    info[i].cycles = i * THREAD_CYCLES;
    if (0 != (err = pthread_create(&thread[i], NULL,
                                   ThreadFunction, &info[i]))) {
      fprintf(stderr, "clock_test: pthread_create failed, error %d\n",
              err);
      num_failures++;
      goto done;
    }
  }

  for (i = 0; i < NACL_ARRAY_SIZE(thread); i++) {
    if (0 != (err = pthread_join(thread[i], NULL))) {
      fprintf(stderr, "clock_test: pthread_join failed, error %d\n",
              err);
      num_failures++;
      goto done;
    }
  }

  if (0 != clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t_process_end)) {
    fprintf(stderr, "clock_test: clock_gettime (now) failed, error %d\n",
            errno);
    num_failures++;
    goto done;
  }

  if (0 != clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t_thread_end)) {
    fprintf(stderr, "clock_test: clock_gettime (now) failed, error %d\n",
            errno);
    num_failures++;
    goto done;
  }

  thread_elapsed =
      (t_thread_end.tv_sec - t_thread_start.tv_sec) * NACL_NANOS_PER_UNIT +
      (t_thread_end.tv_nsec - t_thread_start.tv_nsec);

  process_elapsed =
      (t_process_end.tv_sec - t_process_start.tv_sec) * NACL_NANOS_PER_UNIT +
      (t_process_end.tv_nsec - t_process_start.tv_nsec);

  for (i = 0; i < NACL_ARRAY_SIZE(thread); i++) {
    uint64_t thread_elapsed_nanos;
    uint64_t process_elapsed_nanos;

    if (info[i].num_failures > 0) {
      num_failures += info[i].num_failures;
      goto done;
    }

    thread_elapsed_nanos = info[i].thread_time.tv_sec * NACL_NANOS_PER_UNIT +
        info[i].thread_time.tv_nsec;
    process_elapsed_nanos = info[i].process_time.tv_sec * NACL_NANOS_PER_UNIT +
        info[i].process_time.tv_nsec;
    printf("%zd: thread=%20"PRIu64" nS, process=%20"PRIu64" nS\n",
           i, thread_elapsed_nanos, process_elapsed_nanos);
    child_thread_elapsed += thread_elapsed_nanos;
  }

  elapsed_lower_bound = thread_elapsed + child_thread_elapsed;
  elapsed_upper_bound = (uint64_t) (thread_elapsed +
      child_thread_elapsed * g_fuzzy_factor + g_syscall_overhead);

  printf("thread time:         %20"PRIu64" nS\n", thread_elapsed);
  printf("process time:        %20"PRIu64" nS\n", process_elapsed);
  printf("child thread time:   %20"PRIu64" nS\n", child_thread_elapsed);
  printf("elapsed lower bound: %20"PRIu64" nS\n", elapsed_lower_bound);
  printf("elapsed upper bound: %20"PRIu64" nS\n", elapsed_upper_bound);

  if (process_elapsed < elapsed_lower_bound ||
      elapsed_upper_bound < process_elapsed) {
    printf("discrepancy too large\n");
    num_failures++;
  }
 done:
  printf((0 == num_failures) ? "PASSED\n" : "FAILED\n");
  return num_failures;
}

int main(int ac, char **av) {
  uint64_t  sleep_nanos = DEFAULT_NANOSLEEP_TIME;
  int       opt;
  uint32_t  num_failures = 0;

  while (-1 != (opt = getopt(ac, av, "cf:o:s:S:"))) {
    switch (opt) {
      case 'c':
        g_cputime = 1;
        break;
      case 'f':
        g_fuzzy_factor = strtod(optarg, (char **) NULL);
        break;
      case 'o':
        g_syscall_overhead = strtoul(optarg, (char **) NULL, 0);
        break;
      case 's':
        g_slop_ms = strtoul(optarg, (char **) NULL, 0);
        break;
      case 'S':
        sleep_nanos = strtoul(optarg, (char **) NULL, 0);
        break;
      default:
        fprintf(stderr, "clock_test: unrecognized option `%c'.\n",
                opt);
        fprintf(stderr,
                "Usage: nacl_clock_test [-c] [-f fuzz_factor]\n"
                "       [-s sleep_nanos] [-o syscall_overhead_nanos]\n");
        return -1;
    }
  }

  if (g_cputime) {
    num_failures += ClockCpuTimeAccuracyTest();
  } else {
    num_failures += ClockMonotonicAccuracyTest(sleep_nanos);
    num_failures += ClockRealtimeAccuracyTest();
  }

  return num_failures;
}
