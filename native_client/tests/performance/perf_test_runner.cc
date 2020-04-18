/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/tests/performance/perf_test_compat_osx.h"
#include "native_client/tests/performance/perf_test_runner.h"


double TimeIterations(PerfTest *test, int iterations) {
  struct timespec start_time;
  struct timespec end_time;
  ASSERT_EQ(clock_gettime(CLOCK_MONOTONIC, &start_time), 0);
  for (int i = 0; i < iterations; i++) {
    test->run();
  }
  ASSERT_EQ(clock_gettime(CLOCK_MONOTONIC, &end_time), 0);
  double total_time =
      (end_time.tv_sec - start_time.tv_sec
       + (double) (end_time.tv_nsec - start_time.tv_nsec) / 1e9);
  // Output the raw data.
  printf("  %.3f usec (%g sec) per iteration: %g sec for %i iterations\n",
         total_time / iterations * 1e6,
         total_time / iterations,
         total_time, iterations);
  return total_time;
}

int CalibrateIterationCount(PerfTest *test, double target_time,
                            int sample_count) {
  int calibration_iterations = 100;
  double calibration_time;
  for (;;) {
    calibration_time = TimeIterations(test, calibration_iterations);
    // If the test completed too quickly to get an accurate
    // measurement, try a larger number of iterations.
    if (calibration_time >= 1e-5)
      break;
    ASSERT_LE(calibration_iterations, INT_MAX / 10);
    calibration_iterations *= 10;
  }

  double iterations_d =
      (target_time / (calibration_time / calibration_iterations)
       / sample_count);
  // Sanity checks for very fast or very slow tests.
  ASSERT_LE(iterations_d, INT_MAX);
  int iterations = iterations_d;
  if (iterations < 1)
    iterations = 1;
  return iterations;
}

void TimePerfTest(PerfTest *test, double *mean, double *stddev) {
  // 'target_time' is the amount of time we aim to run this perf test
  // for in total.
  double target_time = 0.5;  // seconds
  // 'sample_count' is the number of separate timings we take in order
  // to measure the variability of the results.
  int sample_count = 5;
  int iterations = CalibrateIterationCount(test, target_time, sample_count);

  double sum = 0;
  double sum_of_squares = 0;
  for (int i = 0; i < sample_count; i++) {
    double time = TimeIterations(test, iterations) / iterations;
    sum += time;
    sum_of_squares += time * time;
  }
  *mean = sum / sample_count;
  *stddev = sqrt(sum_of_squares / sample_count - *mean * *mean);
}

void PerfTestRealTime(const char *description_string, const char *test_name,
                      PerfTest *test, double *result_mean) {
  double mean;
  double stddev;
  printf("Measuring real time:\n");
  TimePerfTest(test, &mean, &stddev);
  printf("  mean:   %.6f usec\n", mean * 1e6);
  printf("  stddev: %.6f usec\n", stddev * 1e6);
  printf("  relative stddev: %.2f%%\n", stddev / mean * 100);
  // Output the result in a format that Buildbot will recognise in the
  // logs and record, using the Chromium perf testing infrastructure.
  printf("RESULT %s: %s= {%.6f, %.6f} us\n",
         test_name, description_string, mean * 1e6, stddev * 1e6);
  *result_mean = mean;
}

#if defined(__i386__) || defined(__x86_64__)

static INLINE uint64_t ReadTimestampCounter() {
  uint32_t edx;  // Top 32 bits of timestamp
  uint32_t eax;  // Bottom 32 bits of timestamp
  // NaCl's x86 validators don't allow rdtscp, so we can't check
  // whether the thread has been moved to a different core.
  __asm__ volatile("rdtsc" : "=d"(edx), "=a"(eax));
  return (((uint64_t) edx) << 32) | eax;
}

static int CompareUint64(const void *val1, const void *val2) {
  uint64_t i1 = *(uint64_t *) val1;
  uint64_t i2 = *(uint64_t *) val2;
  if (i1 == i2)
    return 0;
  return i1 < i2 ? -1 : 1;
}

void PerfTestCycleCount(const char *description_string, const char *test_name,
                        PerfTest *test, uint64_t *result_cycles) {
  printf("Measuring clock cycles:\n");
  uint64_t times[101];
  for (size_t i = 0; i < NACL_ARRAY_SIZE(times); i++) {
    uint64_t start_time = ReadTimestampCounter();
    test->run();
    uint64_t end_time = ReadTimestampCounter();
    times[i] = end_time - start_time;
  }

  // We expect the first run to be slower because caches won't be
  // warm.  We print the first and slowest runs so that we can verify
  // this.
  printf("  first runs (cycles):   ");
  for (size_t i = 0; i < 10; i++)
    printf(" %" PRId64, times[i]);
  printf(" ...\n");

  qsort(times, NACL_ARRAY_SIZE(times), sizeof(times[0]), CompareUint64);

  printf("  slowest runs (cycles):  ...");
  for (size_t i = NACL_ARRAY_SIZE(times) - 10; i < NACL_ARRAY_SIZE(times); i++)
    printf(" %" PRId64, times[i]);
  printf("\n");

  int count = NACL_ARRAY_SIZE(times) - 1;
  uint64_t q1 = times[count * 1 / 4];  // First quartile
  uint64_t q2 = times[count * 1 / 2];  // Median
  uint64_t q3 = times[count * 3 / 4];  // Third quartile
  printf("  min:     %" PRId64 " cycles\n", times[0]);
  printf("  q1:      %" PRId64 " cycles\n", q1);
  printf("  median:  %" PRId64 " cycles\n", q2);
  printf("  q3:      %" PRId64 " cycles\n", q3);
  printf("  max:     %" PRId64 " cycles\n", times[count]);
  // The "{...}" RESULT syntax usually means standard deviation but
  // here we report the interquartile range.
  printf("RESULT %s_CycleCount: %s= {%" PRId64 ", %" PRId64 "} count\n",
         test_name, description_string, q2, q3 - q1);
  *result_cycles = q2;
}

#endif

void RunPerfTest(const char *description_string, const char *test_name,
                 PerfTest *test) {
  printf("\n%s:\n", test_name);
  double mean_time;
  PerfTestRealTime(description_string, test_name, test, &mean_time);
#if defined(__i386__) || defined(__x86_64__)
  uint64_t cycles;
  PerfTestCycleCount(description_string, test_name, test, &cycles);
  // The apparent clock speed can be used to sanity-check the results,
  // e.g. to see whether the CPU is in power-saving mode.
  printf("Apparent clock speed: %.0f MHz\n", cycles / mean_time / 1e6);
#endif
  delete test;
}

int main(int argc, char **argv) {
  const char *description_string = argc >= 2 ? argv[1] : "time";

  // Turn off stdout buffering to aid debugging.
  setvbuf(stdout, NULL, _IONBF, 0);

#define RUN_TEST(class_name) \
    extern PerfTest *Make##class_name(); \
    RunPerfTest(description_string, #class_name, Make##class_name());

  RUN_TEST(TestNull);
#if defined(__native_client__)
  RUN_TEST(TestNaClSyscall);
#endif
#if NACL_LINUX || NACL_OSX
  RUN_TEST(TestHostSyscall);
#endif
  RUN_TEST(TestSetjmpLongjmp);
  RUN_TEST(TestClockGetTime);
#if !NACL_OSX
  RUN_TEST(TestTlsVariable);
#endif
  RUN_TEST(TestMmapAnonymous);
  RUN_TEST(TestAtomicIncrement);
  RUN_TEST(TestUncontendedMutexLock);
  RUN_TEST(TestCondvarSignalNoOp);
  RUN_TEST(TestThreadCreateAndJoin);
  RUN_TEST(TestThreadWakeup);

#if defined(__native_client__)
  // Test untrusted fault handling.  This should come last because, on
  // Windows, registering a fault handler has a performance impact on
  // thread creation and exit.  This is because when the Windows debug
  // exception handler is attached to sel_ldr as a debugger, Windows
  // suspends the whole sel_ldr process every time a thread is created
  // or exits.
  RUN_TEST(TestCatchingFault);
  // Measure that overhead by running MakeTestThreadCreateAndJoin again.
  RunPerfTest(description_string,
              "TestThreadCreateAndJoinAfterSettingFaultHandler",
              MakeTestThreadCreateAndJoin());
#endif

#undef RUN_TEST

  return 0;
}
