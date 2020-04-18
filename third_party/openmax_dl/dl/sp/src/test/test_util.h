/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_ARM_FFT_TEST_UTIL_H_
#define WEBRTC_ARM_FFT_TEST_UTIL_H_

#include <stdio.h>

#include "dl/sp/src/test/compare.h"

/* Command line options */
struct Options {
    /*
     * Set to non-zero if test is only for real signals.  This is just
     * for printing out the correct usage message.
     */
    int real_only_;

    /* Debugging output level, used in test and non-test mode */
    int verbose_;

    /* Test mode where all tests are run. */
    int test_mode_;

    /* Run forward FFT tests (in test mode) */
    int do_forward_tests_;

    /* Run inverse FFT tests (in test mode) */
    int do_inverse_tests_;

    /* Minimum FFT order for test mode */
    int min_fft_order_;

    /* Maximum FFT order for test mode */
    int max_fft_order_;

    /* FFT Order */
    int fft_log_size_;

    /* Forward FFT scale factor, only for for fixed-point routines */
    int scale_factor_;

    /* Signal type to use for testing */
    int signal_type_;

    /* Signal amplitude */
    float signal_value_;

    /* Set if the command line options set a value for signalValue */
    int signal_value_given_;
};

/*
 * Information about a test that is known to fail.
 */
struct KnownTestFailures {
    /* FFT order of the test */
    int fft_order_;

    /* Set to non-zero for inverse FFT case.  Otherwise, it's forward FFT */
    int is_inverse_fft_test_;

    /* The test signal used */
    int signal_type_;
};

struct TestInfo {
    /* True if test is for real signals */
    int real_only_;

    /* Max FFT order to be tested */
    int max_fft_order_;

    /* Min FFT order to be tested */
    int min_fft_order_;

    /* True if forward FFT tests should be run */
    int do_forward_tests_;

    /* True if inverse FFT tests should be run */
    int do_inverse_tests_;

    /* SNR threshold for forward FFT tests */
    float forward_threshold_;

    /* SNR threshold for inverse FFT tests */
    float inverse_threshold_;

    /*
     * Array of known test failures.  Should either be 0 or point to
     * an array of known failures, terminated by a test case with
     * negative fftOrder.
     */
    struct KnownTestFailures* known_failures_;
};

/*
 * Set default options for the command line options.  Must be called
 * before call |processCommandLine|
 */
void SetDefaultOptions(struct Options* options, int real_only,
                       int max_fft_order);

/*
 * Process the command line options
 */
void ProcessCommandLine(struct Options* options, int argc, char* argv[],
                        const char* summary);

/*
 * Print command line options and their values, for debugging.
 */
void DumpOptions(FILE*, const struct Options* options);

/*
 * Run FFT test for one case. May or may not include both forward and
 * inverse tests.
 */
void TestOneFFT(int fft_log_size, int signal_type, float signal_value,
                const struct TestInfo* info, const char* message);

/*
 * Run one forward FFT test of the given size, signal type, and amplitude
 */
float RunOneForwardTest(int fft_log_size, int signal_type,
                        float signal_value, struct SnrResult* snr);

/*
 * Run one inverse FFT test of the given size, signal type, and amplitude
 */
float RunOneInverseTest(int fft_log_size, int signal_type,
                        float signal_value, struct SnrResult* snr);

/*
 * Run all FFT tests, as specified by |info|
 */
int RunAllTests(const struct TestInfo* info);

/*
 * Returns the program name, for debugging.
 */
char* ProgramName(char*);

/*
 * Return true if the specified FFT test is a known failure.
 */
int IsKnownFailure(int fft_order, int is_forward_fft, int signal_type,
                   struct KnownTestFailures* known_failures);

/*
 * Neatly print the contents of an array to stdout.
 */
void DumpArrayReal16(const char* array_name, int count, const OMX_S16* array);
void DumpArrayReal32(const char* array_name, int count, const OMX_S32* array);
void DumpArrayComplex16(const char* array_name, int count,
                        const OMX_SC16* array);
void DumpArrayComplex32(const char* array_name, int count,
                        const OMX_SC32* array);
void DumpArrayFloat(const char* array_name, int count, const OMX_F32* array);
void DumpArrayComplexFloat(const char* array_name, int count,
                           const OMX_FC32* array);
#endif
