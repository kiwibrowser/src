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

#define MAX_FFT_ORDER   12

int verbose = 0;
int signal_value = 32767;
int scale_factor = 0;

struct KnownTestFailures known_failures[] = {
    {11, 0, 1},
    {11, 0, 2},
    {11, 0, 3},
    {12, 0, 1},
    {12, 0, 2},
    {12, 0, 3},
    { 9, 1, 1},
    { 9, 1, 2},
    {10, 1, 1},
    {10, 1, 2},
    {11, 1, 1},
    {11, 1, 2},
    {11, 1, 3},
    {12, 1, 1},
    {12, 1, 2},
    {12, 1, 3},
    /* Marker to terminate array */
    {-1, 0, 0}
};

int main(int argc, char* argv[]) {
  struct Options options;
  struct TestInfo info;

  SetDefaultOptions(&options, 0, MAX_FFT_ORDER);

  options.signal_value_ = signal_value;
  options.scale_factor_ = scale_factor;

  ProcessCommandLine(&options, argc, argv,
                     "Test forward and inverse 16-bit fixed-point FFT\n");

  verbose = options.verbose_;
  signal_value = options.signal_value_;
  scale_factor = options.scale_factor_;

  if (verbose > 255)
    DumpOptions(stderr, &options);

  info.real_only_ = options.real_only_;
  info.max_fft_order_ = options.max_fft_order_;
  info.min_fft_order_ = options.min_fft_order_;
  info.do_forward_tests_ = options.do_forward_tests_;
  info.do_inverse_tests_ = options.do_inverse_tests_;
  info.known_failures_ = known_failures;
  /*
   * These SNR threshold values critically depend on the
   * signal_value that is set for the tests!
   */
  info.forward_threshold_ = 33.01;
  info.inverse_threshold_ = 35.59;

  if (options.test_mode_) {
    RunAllTests(&info);
  } else {
    TestOneFFT(options.fft_log_size_,
               options.signal_type_,
               options.signal_value_,
               &info,
               "16-bit FFT");
  }

  return 0;
}

void GenerateSignal(OMX_SC16* x, struct ComplexFloat* fft,
                    struct ComplexFloat* x_true, int size, int sigtype,
                    int scale_factor) {
  int k;

  GenerateTestSignalAndFFT(x_true, fft, size, sigtype, signal_value, 0);

  /*
   * Convert the complex result to what we want
   */

  for (k = 0; k < size; ++k) {
    x[k].Re = 0.5 + x_true[k].Re;
    x[k].Im = 0.5 + x_true[k].Im;
  }
}

void DumpFFTSpec(OMXFFTSpec_C_SC16* pSpec) {
  ARMsFFTSpec_SC16* p = (ARMsFFTSpec_SC16*) pSpec;
  printf(" N = %d\n", p->N);
  printf(" pBitRev  = %p\n", p->pBitRev);
  printf(" pTwiddle = %p\n", p->pTwiddle);
  printf(" pBuf     = %p\n", p->pBuf);
}

float RunOneForwardTest(int fft_log_size, int signal_type,
                        float unused_signal_value,
                        struct SnrResult* snr) {
  OMX_SC16* x;
  OMX_SC16* y;

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;

  struct ComplexFloat* x_true;
  struct ComplexFloat* y_true;
  OMX_SC16* y_scaled;

  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_C_SC16 * fft_fwd_spec = NULL;
  int fft_size;

  /*
   * With 16-bit numbers, we need to be careful to use all of the
   * available bits to get good accuracy.  Hence, set signal_value to
   * the max 16-bit value (or close to it).
   *
   * To get good FFT results, also set the forward FFT scale factor
   * to be the same as the order.  This was determined by
   * experimentation, so be careful!
   */
  signal_value = 32767;
  scale_factor = fft_log_size;

  fft_size = 1 << fft_log_size;

  status = omxSP_FFTGetBufSize_C_SC16(fft_log_size, &fft_spec_buffer_size);
  if (verbose > 63) {
    printf("bufSize = %d\n", fft_spec_buffer_size);
  }

  fft_fwd_spec = (OMXFFTSpec_C_SC16*) malloc(fft_spec_buffer_size);
  status = omxSP_FFTInit_C_SC16(fft_fwd_spec, fft_log_size);
  if (status) {
    fprintf(stderr, "Failed to init forward FFT:  status = %d\n", status);
    exit(1);
  }

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;

  x_true = (struct ComplexFloat*) malloc(sizeof(*x_true) * fft_size);
  y_true = (struct ComplexFloat*) malloc(sizeof(*y_true) * fft_size);
  y_scaled = (OMX_SC16*) malloc(sizeof(*y_true) * fft_size);

  GenerateSignal(x, y_true, x_true, fft_size, signal_type, scale_factor);

  {
    float scale = pow(2.0, fft_log_size);

    for (n = 0; n < fft_size; ++n) {
      y_scaled[n].Re = 0.5 + y_true[n].Re / scale;
      y_scaled[n].Im = 0.5 + y_true[n].Im / scale;
    }
  }

  if (verbose > 63) {
    printf("Signal\n");
    DumpArrayComplex16("x", fft_size, x);
    printf("Expected FFT output\n");
    DumpArrayComplex16("y", fft_size, y_scaled);
  }

  status = omxSP_FFTFwd_CToC_SC16_Sfs(x, y, fft_fwd_spec, scale_factor);
  if (status) {
    fprintf(stderr, "Forward FFT failed: status = %d\n", status);
    exit(1);
  }

  if (verbose > 63) {
    printf("FFT Output\n");
    DumpArrayComplex16("y", fft_size, y);
  }

  CompareComplex16(snr, y, y_scaled, fft_size);

  return snr->complex_snr_;
}

float RunOneInverseTest(int fft_log_size, int signal_type,
                        float unused_signal_value,
                        struct SnrResult* snr) {
  OMX_SC16* x;
  OMX_SC16* y;
  OMX_SC16* z;
  OMX_SC16* y_scaled;

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;
  struct AlignedPtr* y_scaled_aligned;

  struct ComplexFloat* x_true;
  struct ComplexFloat* y_true;

  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_C_SC16 * fft_fwd_spec = NULL;
  OMXFFTSpec_C_SC16 * fft_inv_spec = NULL;
  int fft_size;

  /*
   * With 16-bit numbers, we need to be careful to use all of the
   * available bits to get good accuracy.  Hence, set signal_value to
   * the max 16-bit value (or close to it).
   *
   * To get good FFT results, also set the forward FFT scale factor
   * to be the same as the order.  This was determined by
   * experimentation, so be careful!
   */
  signal_value = 32767;

  fft_size = 1 << fft_log_size;

  status = omxSP_FFTGetBufSize_C_SC16(fft_log_size, &fft_spec_buffer_size);
  if (verbose > 3) {
    printf("bufSize = %d\n", fft_spec_buffer_size);
  }

  fft_inv_spec = (OMXFFTSpec_C_SC16*)malloc(fft_spec_buffer_size);
  status = omxSP_FFTInit_C_SC16(fft_inv_spec, fft_log_size);
  if (status) {
    fprintf(stderr, "Failed to init backward FFT:  status = %d\n", status);
    exit(1);
  }

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);
  y_scaled_aligned = AllocAlignedPointer(32, sizeof(*y_true) * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;
  y_scaled = y_scaled_aligned->aligned_pointer_;

  y_true = (struct ComplexFloat*) malloc(sizeof(*y_true) * fft_size);
  x_true = (struct ComplexFloat*) malloc(sizeof(*x_true) * fft_size);


  GenerateSignal(x, y_true, x_true, fft_size, signal_type, fft_log_size);

  {
    /*
     * To get max accuracy, scale the input to the inverse FFT up
     * to use as many bits as we can.
     */
    float scale = 1;
    float max = 0;

    for (n = 0; n < fft_size; ++n) {
      float val;
      val = fabs(y_true[n].Re);
      if (val > max) {
        max = val;
      }
      val = fabs(y_true[n].Im);
      if (val > max) {
        max = val;
      }
    }

    scale = 16384 / max;
    if (verbose > 63)
      printf("Inverse FFT input scaled factor %g\n", scale);

    /*
     * Scale both the true FFT signal and the input so we can
     * compare them correctly later
     */
    for (n = 0; n < fft_size; ++n) {
      y_scaled[n].Re = 0.5 + y_true[n].Re * scale;
      y_scaled[n].Im = 0.5 + y_true[n].Im * scale;
      x_true[n].Re *= scale;
      x_true[n].Im *= scale;
    }
  }


  if (verbose > 63) {
    printf("Inverse FFT Input Signal\n");
    DumpArrayComplex16("yScaled", fft_size, y_scaled);
    printf("Expected Inverse FFT Output\n");
    DumpArrayComplexFloat("x_true", fft_size, (OMX_FC32*) x_true);
  }

  status = omxSP_FFTInv_CToC_SC16_Sfs(y_scaled, z, fft_inv_spec, 0);

  if (verbose > 7)
    printf("Inverse FFT scaling = %d\n", status);

  if (verbose > 127) {
    printf("Raw Inverse FFT Output\n");
    DumpArrayComplex16("z", fft_size, z);
  }

  /*
   * The inverse FFT routine returns how much scaling was done. To
   * compare the output with the expected output, we need to scale
   * the expected output according to the scale factor returned.
   */
  for (n = 0; n < fft_size; ++n) {
    x[n].Re = 0.5 + x_true[n].Re;
    x[n].Im = 0.5 + x_true[n].Im;
  }

  if (verbose > 63) {
    printf("Inverse FFT Output\n");
    printf(" Actual\n");
    DumpArrayComplex16("z", fft_size, z);
    printf(" Expected (scaled)\n");
    DumpArrayComplex16("x", fft_size, x);
  }

  CompareComplex16(snr, z, x, fft_size);

  return snr->complex_snr_;
}
