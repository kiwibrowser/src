/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "dl/sp/src/test/test_util.h"

#include "dl/sp/src/test/compare.h"
#include "dl/sp/src/test/gensig.h"

/*
 * Test results from running either forward or inverse FFT tests
 */
struct TestResult {
  /* Number of tests that failed */
  int failed_count_;

  /* Number of tests run */
  int test_count_;

  /* Number of tests that were expected to fail */
  int expected_failure_count_;

  /* Number of tests that were expected to fail but didn't */
  int unexpected_pass_count_;

  /* Number of tests that unexpectedly failed */
  int unexpected_failure_count_;

  /* The minimum SNR found for all of the tests */
  float min_snr_;
};

/*
 * Run one FFT test
 */
void TestOneFFT(int fft_log_size,
                int signal_type,
                float signal_value,
                const struct TestInfo* info,
                const char* message) {
  struct SnrResult snr;

  if (info->do_forward_tests_) {
    RunOneForwardTest(fft_log_size, signal_type, signal_value, &snr);
    printf("Forward %s\n", message);
    printf("SNR:  real part    %10.3f dB\n", snr.real_snr_);
    printf("      imag part    %10.3f dB\n", snr.imag_snr_);
    printf("      complex part %10.3f dB\n", snr.complex_snr_);
  }

  if (info->do_inverse_tests_) {
    RunOneInverseTest(fft_log_size, signal_type, signal_value, &snr);
    printf("Inverse %s\n", message);
    if (info->real_only_) {
      printf("SNR:  real         %10.3f dB\n", snr.real_snr_);
    } else {
      printf("SNR:  real part    %10.3f dB\n", snr.real_snr_);
      printf("      imag part    %10.3f dB\n", snr.imag_snr_);
      printf("      complex part %10.3f dB\n", snr.complex_snr_);
    }
  }
}

/*
 * Run a set of tests, printing out the result of each test.
 */
void RunTests(struct TestResult* result,
              float (*test_function)(int, int, float, struct SnrResult*),
              const char* id,
              int is_inverse_test,
              const struct TestInfo* info,
              float snr_threshold) {
  int fft_order;
  int signal_type;
  float snr;
  int tests = 0;
  int failures = 0;
  int expected_failures = 0;
  int unexpected_failures = 0;
  int unexpected_passes = 0;
  float min_snr = 1e10;
  struct SnrResult snrResults;

  for (fft_order = info->min_fft_order_; fft_order <= info->max_fft_order_;
       ++fft_order) {
    for (signal_type = 0; signal_type < MaxSignalType(info->real_only_);
         ++signal_type) {
      int known_failure = 0;
      int test_failed = 0;
      ++tests;
      snr = test_function(fft_order, signal_type, 1024.0, &snrResults);
      if (snr < min_snr)
        min_snr = snr;
      known_failure = IsKnownFailure(fft_order, is_inverse_test,
                                     signal_type, info->known_failures_);
      if (snr < snr_threshold) {
        ++failures;
        test_failed = 1;
        if (known_failure) {
          ++expected_failures;
          printf(" *FAILED: %s ", id);
        } else {
          ++unexpected_failures;
          printf("**FAILED: %s ", id);
        }
      } else {
        test_failed = 0;
        printf("  PASSED: %s ", id);
      }
      printf("order %2d signal %d:  SNR = %9.3f",
             fft_order, signal_type, snr);
      if (known_failure) {
        if (test_failed) {
          printf(" (expected failure)");
        } else {
          ++unexpected_passes;
          printf(" (**Expected to fail, but passed)");
        }
      }
      printf("\n");
    }
  }

  printf("%sSummary:  %d %s tests failed out of %d tests. "
         "(Success rate %.2f%%.)\n",
         failures ? "**" : "",
         failures,
         id,
         tests,
         (100.0 * (tests - failures)) / tests);
  if (expected_failures || unexpected_passes || unexpected_failures) {
    printf("    (%d expected failures)\n", expected_failures);
    printf("    (%d unexpected failures)\n", unexpected_failures);
    printf("    (%d unexpected passes)\n", unexpected_passes);
  }
  
  printf("    (Minimum SNR = %.3f dB)\n", min_snr);

  result->failed_count_ = failures;
  result->test_count_ = tests;
  result->expected_failure_count_ = expected_failures;
  result->unexpected_pass_count_ = unexpected_passes;
  result->unexpected_failure_count_ = unexpected_failures;
  result->min_snr_ = min_snr;
}

/*
 * For all FFT orders and signal types, run the forward FFT.
 * runOneForwardTest must be defined to compute the forward FFT and
 * return the SNR beween the actual and expected FFT.
 *
 * Also finds the minium SNR from all of the tests and returns the
 * minimum SNR value.
 */
void RunForwardTests(struct TestResult* result, const struct TestInfo* info,
                     float snr_threshold) {
  RunTests(result, RunOneForwardTest, "FwdFFT", 0, info, snr_threshold);
}

void initializeTestResult(struct TestResult *result) {
  result->failed_count_ = 0;
  result->test_count_ = 0;
  result->expected_failure_count_ = 0;
  result->min_snr_ = 1000;
}

/*
 * For all FFT orders and signal types, run the inverse FFT.
 * runOneInverseTest must be defined to compute the forward FFT and
 * return the SNR beween the actual and expected FFT.
 *
 * Also finds the minium SNR from all of the tests and returns the
 * minimum SNR value.
 */
void RunInverseTests(struct TestResult* result, const struct TestInfo* info,
                     float snr_threshold) {
  RunTests(result, RunOneInverseTest, "InvFFT", 1, info, snr_threshold);
}

/*
 * Run all forward and inverse FFT tests, printing a summary of the
 * results.
 */
int RunAllTests(const struct TestInfo* info) {
  int failed;
  int total;
  float min_forward_snr;
  float min_inverse_snr;
  struct TestResult forward_results;
  struct TestResult inverse_results;

  initializeTestResult(&forward_results);
  initializeTestResult(&inverse_results);

  if (info->do_forward_tests_)
    RunForwardTests(&forward_results, info, info->forward_threshold_);
  if (info->do_inverse_tests_)
    RunInverseTests(&inverse_results, info, info->inverse_threshold_);

  failed = forward_results.failed_count_ + inverse_results.failed_count_;
  total = forward_results.test_count_ + inverse_results.test_count_;
  min_forward_snr = forward_results.min_snr_;
  min_inverse_snr = inverse_results.min_snr_;

  if (total) {
    printf("%sTotal: %d tests failed out of %d tests.  "
           "(Success rate = %.2f%%.)\n",
           failed ? "**" : "",
           failed,
           total,
           (100.0 * (total - failed)) / total);
    if (forward_results.expected_failure_count_
        + inverse_results.expected_failure_count_) {
      printf("  (%d expected failures)\n",
             forward_results.expected_failure_count_
             + inverse_results.expected_failure_count_);
      printf("  (%d unexpected failures)\n",
             forward_results.unexpected_failure_count_
             + inverse_results.unexpected_failure_count_);
      printf("  (%d unexpected passes)\n",
             forward_results.unexpected_pass_count_
             + inverse_results.unexpected_pass_count_);
    }
    printf("  Min forward SNR = %.3f dB, min inverse SNR = %.3f dB\n",
           min_forward_snr,
           min_inverse_snr);
  } else {
    printf("No tests run\n");
  }

  return failed;
}
