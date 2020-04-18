/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include "native_client/src/shared/platform/nacl_clock.h"

#include "native_client/src/shared/platform/platform_init.h"

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"

/*
 * Very basic sanity check.  With clock functionality, tests are a
 * pain without a set of globally consistent dependency injection for
 * syscalls, since faking out time-related syscalls in the test
 * without faking out the same syscalls used by other modules is
 * difficult.  Furthermore, this test is trying to verify basic
 * functionality -- and testing against a mock interface that works
 * according to our expectations of what the syscalls will do isn't
 * the same: our assumptions might be wrong, and we ought to have a
 * test that verifies end-to-end functionality.  Here, we just compare
 * clock_gettime of the realtime clock against gettimeofday, and do
 * two monotonic clock samples between a nanosleep to verify that the
 * monotonic clock *approximately* measured the sleep duration -- with
 * great fuzziness.
 *
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
int      g_cputime = 0;
double   g_fuzzy_factor = DEFAULT_NANOSLEEP_EXTRA_FACTOR;
uint64_t g_syscall_overhead = DEFAULT_NANOSLEEP_EXTRA_OVERHEAD;
uint64_t g_slop_ms = 0;

/*
 * ClockMonotonicAccuracyTest samples the NACL_ABI_CLOCK_MONOTONIC
 * clock before and after invoking NaClNanosleep and computes the time
 * delta.  The test is considered to pass if the time delta is close
 * to the requested value.  "Close" is a per-host-OS attribute, thus
 * the above testing parameters.
 */
static int ClockMonotonicAccuracyTest(uint64_t sleep_nanos) {
  int                       num_failures = 0;

  int                       err;
  struct nacl_abi_timespec  t_start;
  struct nacl_abi_timespec  t_sleep;
  struct nacl_abi_timespec  t_end;

  uint64_t                  elapsed_nanos;
  uint64_t                  elapsed_lower_bound;
  uint64_t                  elapsed_upper_bound;

  t_sleep.tv_sec  = sleep_nanos / NACL_NANOS_PER_UNIT;
  t_sleep.tv_nsec = sleep_nanos % NACL_NANOS_PER_UNIT;

  printf("\nCLOCK_MONOTONIC accuracy test:\n");

  if (0 != (err = NaClClockGetTime(NACL_ABI_CLOCK_MONOTONIC, &t_start))) {
    fprintf(stderr,
            "nacl_clock_test: NaClClockGetTime (start) failed, error %d\n",
            err);
    ++num_failures;
    goto done;
  }
  for (;;) {
    err = NaClNanosleep(&t_sleep, &t_sleep);
    if (0 == err) {
      break;
    }
    if (-NACL_ABI_EINTR == err) {
      /* interrupted syscall: sleep some more */
      continue;
    }
    fprintf(stderr,
            "nacl_clock_test: NaClNanoSleep failed, error %d\n", err);
    num_failures++;
    goto done;
  }
  if (0 != (err = NaClClockGetTime(NACL_ABI_CLOCK_MONOTONIC, &t_end))) {
    fprintf(stderr,
            "nacl_clock_test: NaClClockGetTime (end) failed, error %d\n",
            err);
    return 1;
  }

  elapsed_nanos = (t_end.tv_sec - t_start.tv_sec) * NACL_NANOS_PER_UNIT +
      (t_end.tv_nsec - t_start.tv_nsec) + g_slop_ms * NACL_NANOS_PER_MILLI;

  elapsed_lower_bound = sleep_nanos;
  elapsed_upper_bound = (uint64_t) (sleep_nanos * g_fuzzy_factor +
                                    g_syscall_overhead);

  printf("requested sleep:      %20"NACL_PRIu64" nS\n", sleep_nanos);
  printf("actual elapsed sleep: %20"NACL_PRIu64" nS\n", elapsed_nanos);
  printf("sleep lower bound:    %20"NACL_PRIu64" nS\n", elapsed_lower_bound);
  printf("sleep upper bound:    %20"NACL_PRIu64" nS\n", elapsed_upper_bound);

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
 * NACL_ABI_CLOCK_REALTIME against that returned by NaClGetTimeOfDay.
 */
static int ClockRealtimeAccuracyTest(void) {
  int                       num_failures = 0;

  int                       err;
  struct nacl_abi_timespec  t_now_ts;
  struct nacl_abi_timeval   t_now_tv;

  uint64_t                  t_now_ts_nanos;
  uint64_t                  t_now_tv_nanos;
  int64_t                   t_now_diff_nanos;

  printf("\nCLOCK_REALTIME accuracy test:\n");

  if (0 != (err = NaClClockGetTime(NACL_ABI_CLOCK_REALTIME, &t_now_ts))) {
    fprintf(stderr,
            "nacl_clock_test: NaClClockGetTime (now) failed, error %d\n",
            err);
    num_failures++;
    goto done;
  }
  if (0 != (err = NaClGetTimeOfDay(&t_now_tv))) {
    fprintf(stderr,
            "nacl_clock_test: NaClGetTimeOfDay (now) failed, error %d\n",
            err);
    num_failures++;
    goto done;
  }

  t_now_ts_nanos = t_now_ts.tv_sec * NACL_NANOS_PER_UNIT + t_now_ts.tv_nsec;
  t_now_tv_nanos = t_now_tv.nacl_abi_tv_sec * NACL_NANOS_PER_UNIT +
      t_now_tv.nacl_abi_tv_usec * NACL_NANOS_PER_MICRO;

  printf("clock_gettime:   %20"NACL_PRIu64" nS\n", t_now_ts_nanos);
  printf("gettimeofday:    %20"NACL_PRIu64" nS\n", t_now_tv_nanos);

  t_now_diff_nanos = t_now_ts_nanos - t_now_tv_nanos;
  if (t_now_diff_nanos < 0) {
    t_now_diff_nanos = -t_now_diff_nanos;
  }
  printf("time difference: %20"NACL_PRId64" nS\n", t_now_diff_nanos);

  if (t_now_ts_nanos < g_syscall_overhead) {
    printf("discrepancy too large\n");
    num_failures++;
  }
 done:
  printf((0 == num_failures) ? "PASSED\n" : "FAILED\n");
  return num_failures;
}

#ifndef NACL_NO_CPUTIME_TEST
struct ThreadInfo {
  size_t                    cycles;
  struct nacl_abi_timespec  thread_time;
  struct nacl_abi_timespec  process_time;
  int                       num_failures;
};

void WINAPI ThreadFunction(void *ptr) {
  int                       err;

  size_t                    i;
  struct ThreadInfo         *info = (struct ThreadInfo *) ptr;

  for (i = 1; i < info->cycles; i++) {
#if defined(__GNUC__)
    __asm__ volatile("" ::: "memory");
#elif NACL_WINDOWS
    _ReadWriteBarrier();
#else
# error Unsupported platform
#endif
  }

  if (0 != (err = NaClClockGetTime(NACL_ABI_CLOCK_THREAD_CPUTIME_ID,
                                   &info->thread_time))) {
    fprintf(stderr,
            "nacl_clock_test: NaClClockGetTime (now) failed, error %d\n",
            err);
    info->num_failures++;
    return;
  }

  if (0 != (err = NaClClockGetTime(NACL_ABI_CLOCK_PROCESS_CPUTIME_ID,
                                   &info->process_time))) {
    fprintf(stderr,
            "nacl_clock_test: NaClClockGetTime (now) failed, error %d\n",
            err);
    info->num_failures++;
    return;
  }
}

static int ClockCpuTimeAccuracyTest(void) {
  int                       num_failures = 0;

  int                       err;
  struct nacl_abi_timespec  t_process_start;
  struct nacl_abi_timespec  t_process_end;
  struct nacl_abi_timespec  t_thread_start;
  struct nacl_abi_timespec  t_thread_end;

  uint64_t                  thread_elapsed = 0;
  uint64_t                  process_elapsed = 0;
  uint64_t                  child_thread_elapsed = 0;
  uint64_t                  elapsed_lower_bound;
  uint64_t                  elapsed_upper_bound;

  size_t                    i;
  struct ThreadInfo         info[10];
  struct NaClThread         thread[10];

  printf("\nCLOCK_PROCESS/THREAD_CPUTIME_ID accuracy test:\n");

  if (0 != (err = NaClClockGetTime(NACL_ABI_CLOCK_THREAD_CPUTIME_ID,
                                   &t_thread_start))) {
    fprintf(stderr,
            "nacl_clock_test: NaClClockGetTime (now) failed, error %d\n",
            err);
    num_failures++;
    goto done;
  }

  if (0 != (err = NaClClockGetTime(NACL_ABI_CLOCK_PROCESS_CPUTIME_ID,
                                   &t_process_start))) {
    fprintf(stderr,
            "nacl_clock_test: NaClClockGetTime (now) failed, error %d\n",
            err);
    num_failures++;
    goto done;
  }

  for (i = 0; i < NACL_ARRAY_SIZE(thread); i++) {
    memset(&info[i], 0, sizeof info[i]);
    info[i].cycles = i * THREAD_CYCLES;
    if (!NaClThreadCreateJoinable(&thread[i], ThreadFunction, &info[i],
                                  65536)) {
      fprintf(stderr,
              "nacl_clock_test: NaClThreadCreateJoinable failed\n");
      num_failures++;
      goto done;
    }
  }

  for (i = 0; i < NACL_ARRAY_SIZE(thread); i++) {
    NaClThreadJoin(&thread[i]);
  }

  if (0 != (err = NaClClockGetTime(NACL_ABI_CLOCK_PROCESS_CPUTIME_ID,
                                   &t_process_end))) {
    fprintf(stderr,
            "nacl_clock_test: NaClClockGetTime (now) failed, error %d\n",
            err);
    num_failures++;
    goto done;
  }

  if (0 != (err = NaClClockGetTime(NACL_ABI_CLOCK_THREAD_CPUTIME_ID,
                                   &t_thread_end))) {
    fprintf(stderr,
            "nacl_clock_test: NaClClockGetTime (now) failed, error %d\n",
            err);
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
    printf("%"NACL_PRIdS": thread=%20"NACL_PRIu64" nS, process=%20"
           NACL_PRIu64" nS\n",
           i, thread_elapsed_nanos, process_elapsed_nanos);
    child_thread_elapsed += thread_elapsed_nanos;
  }

  elapsed_lower_bound = thread_elapsed + child_thread_elapsed;
  elapsed_upper_bound = (uint64_t) (thread_elapsed +
      child_thread_elapsed * g_fuzzy_factor + g_syscall_overhead);

  printf("thread time:         %20"NACL_PRIu64" nS\n", thread_elapsed);
  printf("process time:        %20"NACL_PRIu64" nS\n", process_elapsed);
  printf("child thread time:   %20"NACL_PRIu64" nS\n", child_thread_elapsed);
  printf("elapsed lower bound: %20"NACL_PRIu64" nS\n", elapsed_lower_bound);
  printf("elapsed upper bound: %20"NACL_PRIu64" nS\n", elapsed_upper_bound);

  if (process_elapsed < elapsed_lower_bound ||
      elapsed_upper_bound < process_elapsed) {
    printf("discrepancy too large\n");
    num_failures++;
  }
 done:
  printf((0 == num_failures) ? "PASSED\n" : "FAILED\n");
  return num_failures;
}
#endif

int main(int ac, char **av) {
  uint64_t                  sleep_nanos = DEFAULT_NANOSLEEP_TIME;

  int                       opt;
  uint32_t                  num_failures = 0;

  puts("This is a basic functionality test being repurposed as an");
  puts(" unit/regression test.  The test parameters default to values that");
  puts("are more appropriate for heavily loaded continuous-testing robots.");
  printf("\nThe default values are:\n -S %"NACL_PRIu64
         " -f %f -o %"NACL_PRIu64" -s %"NACL_PRIu64"\n\n",
         sleep_nanos, g_fuzzy_factor, g_syscall_overhead, g_slop_ms);
  puts("For testing functionality, this test should be run with a different");
  puts("set of parameters.  On an unloaded i7, a sleep duration (-S) of");
  puts("1000000 ns (one millisecond), with a fuzziness factor (-f) of 1.25,");
  puts("a constant test overhead of 100000 ns (100 us), and a");
  puts("sleep duration \"slop\" (-s) of 0 is fine. The CPU time tests has to");
  puts("be explicitly enabled (-c) its run time is significant.");

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
        fprintf(stderr, "nacl_clock_test: unrecognized option `%c'.\n",
                opt);
        fprintf(stderr,
                "Usage: nacl_clock_test [-c] [-f fuzz_factor]\n"
                "       [-s sleep_nanos] [-o syscall_overhead_nanos]\n");
        return -1;
    }
  }

  NaClPlatformInit();

  if (g_cputime) {
    num_failures += ClockCpuTimeAccuracyTest();
  } else {
    num_failures += ClockMonotonicAccuracyTest(sleep_nanos);
    num_failures += ClockRealtimeAccuracyTest();
  }

  NaClPlatformFini();

  return num_failures;
}
