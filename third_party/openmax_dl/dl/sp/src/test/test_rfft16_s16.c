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

int main(int argc, char* argv[]) {
  struct Options options;
  struct TestInfo info;

  SetDefaultOptions(&options, 1, MAX_FFT_ORDER);

  options.signal_value_ = signal_value;
  options.scale_factor_ = scale_factor;

  ProcessCommandLine(&options, argc, argv, "Test forward and inverse real 16 \
                     -bit fixed-point FFT, with 16-bit complex FFT routines\n");

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
  /* No known failures */
  info.known_failures_ = 0;
  info.forward_threshold_ = 45;
  info.inverse_threshold_ = 14;

  if (options.test_mode_) {
    RunAllTests(&info);
  } else {
    TestOneFFT(options.fft_log_size_,
               options.signal_type_,
               options.signal_value_,
               &info,
               "16-bit Real FFT using 16-bit complex FFT");
  }

  return 0;
}

void GenerateSignal(struct ComplexFloat* fft,
                    float* x_true, int size, int sigtype) {
  int k;
  struct ComplexFloat *test_signal;

  test_signal = (struct ComplexFloat*) malloc(sizeof(*test_signal) * size);
  GenerateTestSignalAndFFT(test_signal, fft, size, sigtype, signal_value, 1);

  /*
   * Convert the complex result to what we want
   */

  for (k = 0; k < size; ++k) {
    x_true[k] = test_signal[k].Re;
  }

  free(test_signal);
}

float RunOneForwardTest(int fft_log_size, int signal_type,
                        float unused_signal_value,
                        struct SnrResult* snr) {
  OMX_S16* x;
  OMX_SC16* y;

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;

  float* x_true;
  struct ComplexFloat* y_true;
  OMX_SC16* y_scaled;

  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_R_S16 * fft_fwd_spec = NULL;
  int fft_size;

  /*
   * To get good FFT results, set the forward FFT scale factor
   * to be the same as the order.
   */
  scale_factor = fft_log_size;

  fft_size = 1 << fft_log_size;

  status = omxSP_FFTGetBufSize_R_S16(fft_log_size, &fft_spec_buffer_size);
  if (verbose > 63) {
    printf("fft_spec_buffer_size = %d\n", fft_spec_buffer_size);
  }

  fft_fwd_spec = (OMXFFTSpec_R_S16*) malloc(fft_spec_buffer_size);
  status = omxSP_FFTInit_R_S16(fft_fwd_spec, fft_log_size);
  if (status) {
    fprintf(stderr, "Failed to init forward FFT:  status = %d\n", status);
    exit(1);
  }

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;

  x_true = (float*) malloc(sizeof(*x_true) * fft_size);
  y_true = (struct ComplexFloat*) malloc(sizeof(*y_true) * (fft_size / 2 + 1));
  y_scaled = (OMX_SC16*) malloc(sizeof(*y_true) * (fft_size / 2 + 1));

  GenerateSignal(y_true, x_true, fft_size, signal_type);
  for (n = 0; n < fft_size; ++n) {
    x[n] = 0.5 + x_true[n];
  }

  {
    float scale = 1 << fft_log_size;

    for (n = 0; n < fft_size; ++n) {
      y_scaled[n].Re = 0.5 + y_true[n].Re / scale;
      y_scaled[n].Im = 0.5 + y_true[n].Im / scale;
    }
  }

  if (verbose > 63) {
    printf("Signal\n");
    DumpArrayReal16("x", fft_size, x);

    printf("Expected FFT output\n");
    DumpArrayComplex16("y", fft_size / 2 + 1, y_scaled);
  }

  status = omxSP_FFTFwd_RToCCS_S16_Sfs(x, (OMX_S16*) y, fft_fwd_spec, scale_factor);
  if (status) {
    fprintf(stderr, "Forward FFT failed: status = %d\n", status);
    exit(1);
  }

  if (verbose > 63) {
    printf("FFT Output\n");
    DumpArrayComplex16("y", fft_size / 2 + 1, y);
  }

  CompareComplex16(snr, y, y_scaled, fft_size / 2 + 1);

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  free(fft_fwd_spec);

  return snr->complex_snr_;
}

float RunOneInverseTest(int fft_log_size, int signal_type,
                        float unused_signal_value,
                        struct SnrResult* snr) {
  OMX_S16* x_scaled;
  OMX_S16* z;
  OMX_SC16* y;
  OMX_SC16* y_scaled;

  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;

  float* x_true;
  struct ComplexFloat* y_true;

  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_R_S16 * fft_inv_spec = NULL;
  int fft_size;

  fft_size = 1 << fft_log_size;

  status = omxSP_FFTGetBufSize_R_S16(fft_log_size, &fft_spec_buffer_size);
  if (verbose > 3) {
    printf("fft_spec_buffer_size = %d\n", fft_spec_buffer_size);
  }

  fft_inv_spec = (OMXFFTSpec_R_S16*)malloc(fft_spec_buffer_size);
  status = omxSP_FFTInit_R_S16(fft_inv_spec, fft_log_size);
  if (status) {
    fprintf(stderr, "Failed to init backward FFT:  status = %d\n", status);
    exit(1);
  }

  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size / 2 + 1));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);

  x_true = (float*) malloc(sizeof(*x_true) * fft_size);
  x_scaled = (OMX_S16*) malloc(sizeof(*x_scaled) * fft_size);
  y_true = (struct ComplexFloat*) malloc(sizeof(*y_true) * fft_size);
  y_scaled = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  GenerateSignal(y_true, x_true, fft_size, signal_type);

  {
    /*
     * To get max accuracy, scale the input to the inverse FFT up
     * to use as many bits as we can.
     */
    float scale = 1;
    float max = 0;

    for (n = 0; n < fft_size / 2 + 1; ++n) {
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
    for (n = 0; n < fft_size / 2 + 1; ++n) {
      y_scaled[n].Re = (OMX_S16)(0.5 + y_true[n].Re * scale);
      y_scaled[n].Im = (OMX_S16)(0.5 + y_true[n].Im * scale);
    }
    for (n = 0; n < fft_size; ++n) {
      x_scaled[n] = 0.5 + x_true[n] * scale;
    }
  }


  if (verbose > 63) {
    printf("Inverse FFT Input Signal\n");
    DumpArrayComplex16("y", fft_size / 2 + 1, y_scaled);

    printf("Expected Inverse FFT output\n");
    DumpArrayReal16("x", fft_size, x_scaled);
  }

  status = omxSP_FFTInv_CCSToR_S16_Sfs((OMX_S16 const *)y_scaled, z, fft_inv_spec, 0);
  if (status) {
    fprintf(stderr, "Inverse FFT failed: status = %d\n", status);
    exit(1);
  }

  if (verbose > 63) {
    printf("Actual Inverse FFT Output\n");
    DumpArrayReal16("z", fft_size, z);
  }

  CompareReal16(snr, z, x_scaled, fft_size);

  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  free(fft_inv_spec);

  return snr->real_snr_;
}
