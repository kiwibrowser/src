/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/aligned_ptr.h"
#include "dl/sp/src/test/compare.h"
#include "dl/sp/src/test/gensig.h"
#include "dl/sp/src/test/test_util.h"

int verbose;
int signal_value;

#define MAX_FFT_ORDER   12

int main(int argc, char* argv[]) {
  struct Options options;
  struct TestInfo info;

  SetDefaultOptions(&options, 1, MAX_FFT_ORDER);

  ProcessCommandLine(&options, argc, argv,
                     "Test forward and inverse real 32-bit fixed-point FFT\n");

  verbose = options.verbose_;
  signal_value = options.signal_value_;

  if (verbose > 255)
    DumpOptions(stderr, &options);

  info.real_only_ = options.real_only_;
  info.min_fft_order_ = options.min_fft_order_;
  info.max_fft_order_ = options.max_fft_order_;
  info.do_forward_tests_ = options.do_forward_tests_;
  info.do_inverse_tests_ = options.do_inverse_tests_;
  /* No known failures */
  info.known_failures_ = 0;
  /* These thresholds are set for the default signal_value below */
  info.forward_threshold_ = 105.94;
  info.inverse_threshold_ = 104.62;

  if (!options.signal_value_given_) {
    signal_value = 262144;
  }

  if (options.test_mode_) {
    RunAllTests(&info);
  } else {
    TestOneFFT(options.fft_log_size_,
               options.signal_type_,
               options.signal_value_,
               &info,
               "32-bit Real FFT");
  }

  return 0;
}

void GenerateSignal(OMX_S32* x, OMX_SC32* fft, int size, int signal_type) {
  int k;
  struct ComplexFloat *test_signal;
  struct ComplexFloat *true_fft;

  test_signal = (struct ComplexFloat*) malloc(sizeof(*test_signal) * size);
  true_fft = (struct ComplexFloat*) malloc(sizeof(*true_fft) * size);
  GenerateTestSignalAndFFT(test_signal, true_fft, size,
                           signal_type, signal_value, 1);

  /*
   * Convert the complex result to what we want
   */

  for (k = 0; k < size; ++k) {
    x[k] = 0.5 + test_signal[k].Re;
  }

  for (k = 0; k < size / 2 + 1; ++k) {
    fft[k].Re = 0.5 + true_fft[k].Re;
    fft[k].Im = 0.5 + true_fft[k].Im;
  }

  free(test_signal);
  free(true_fft);
}

float RunOneForwardTest(int fft_log_size, int signal_type, float signal_value,
                        struct SnrResult* snr) {
  OMX_S32* x;
  OMX_SC32* y;

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;

  OMX_SC32* y_true;

  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_R_S32 * pFwdSpec = NULL;
  int fft_size;

  fft_size = 1 << fft_log_size;

  status = omxSP_FFTGetBufSize_R_S32(fft_log_size, &fft_spec_buffer_size);
  if (verbose > 63) {
    printf("fft_spec_buffer_size = %d\n", fft_spec_buffer_size);
  }

  pFwdSpec = (OMXFFTSpec_R_S32*) malloc(fft_spec_buffer_size);
  status = omxSP_FFTInit_R_S32(pFwdSpec, fft_log_size);
  if (status) {
    fprintf(stderr, "Failed to init forward FFT:  status = %d\n", status);
    exit(1);
  }

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  y_true = (OMX_SC32*) malloc(sizeof(*y_true) * (fft_size / 2 + 1));

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;

  GenerateSignal(x, y_true, fft_size, signal_type);

  if (verbose > 63) {
    printf("Signal\n");
    DumpArrayReal32("x", fft_size, x);

    printf("Expected FFT output\n");
    DumpArrayComplex32("y", fft_size / 2, y_true);
  }

  status = omxSP_FFTFwd_RToCCS_S32_Sfs(x, (OMX_S32*) y, pFwdSpec, 0);
  if (status) {
    fprintf(stderr, "Forward FFT failed: status = %d\n", status);
    exit(1);
  }

  if (verbose > 63) {
    printf("FFT Output\n");
    DumpArrayComplex32("y", fft_size / 2, y);
  }

  CompareComplex32(snr, y, y_true, fft_size / 2 + 1);

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  free(y_true);
  free(pFwdSpec);

  return snr->complex_snr_;
}

float RunOneInverseTest(int fft_log_size, int signal_type, float signal_value,
                        struct SnrResult* snr) {
  OMX_S32* x;
  OMX_SC32* y;
  OMX_S32* z;
  OMX_SC32* y_true;

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;
  struct AlignedPtr* y_true_aligned;

  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_R_S32 * pFwdSpec = NULL;
  OMXFFTSpec_R_S32 * pInvSpec = NULL;
  int fft_size;

  fft_size = 1 << fft_log_size;

  status = omxSP_FFTGetBufSize_R_S32(fft_log_size, &fft_spec_buffer_size);
  if (verbose > 3) {
    printf("fft_spec_buffer_size = %d\n", fft_spec_buffer_size);
  }

  pInvSpec = (OMXFFTSpec_R_S32*)malloc(fft_spec_buffer_size);
  status = omxSP_FFTInit_R_S32(pInvSpec, fft_log_size);
  if (status) {
    fprintf(stderr, "Failed to init backward FFT:  status = %d\n", status);
    exit(1);
  }

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size / 2 + 1));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);
  y_true_aligned = AllocAlignedPointer(32,
                                       sizeof(*y_true) * (fft_size / 2 + 1));

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;
  y_true = y_true_aligned->aligned_pointer_;

  GenerateSignal(x, y_true, fft_size, signal_type);

  if (verbose > 63) {
    printf("Inverse FFT Input Signal\n");
    DumpArrayComplex32("y", fft_size / 2, y_true);

    printf("Expected Inverse FFT output\n");
    DumpArrayReal32("x", fft_size, x);
  }

  status = omxSP_FFTInv_CCSToR_S32_Sfs((OMX_S32*) y_true, z, pInvSpec, 0);
  if (status) {
    fprintf(stderr, "Inverse FFT failed: status = %d\n", status);
    exit(1);
  }

  if (verbose > 63) {
    printf("Actual Inverse FFT Output\n");
    DumpArrayReal32("x", fft_size, z);
  }

  CompareReal32(snr, z, x, fft_size);

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  FreeAlignedPointer(y_true_aligned);
  free(pInvSpec);

  return snr->real_snr_;
}
