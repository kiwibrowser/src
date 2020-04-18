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

#define MAX_FFT_ORDER   TWIDDLE_TABLE_ORDER

int verbose;

extern const char* UsageMessage();
extern void FinishedMessage();
extern void SetThresholds(struct TestInfo *info);
extern OMXResult ForwardFFT(OMX_FC32* x,
                            OMX_FC32* y,
                            OMXFFTSpec_R_F32 *fft_fwd_spec);
extern OMXResult InverseFFT(OMX_FC32* y,
                            OMX_FC32* z,
                            OMXFFTSpec_R_F32 *fft_inv_spec);

void TestFloatFFT(int fft_log_size,
                  int sigtype,
                  float signal_value,
                  const struct TestInfo* info);

int main(int argc, char* argv[]) {
  struct Options options;
  struct TestInfo info;
  int failed_count = 0;

  SetDefaultOptions(&options, 0, MAX_FFT_ORDER);

  ProcessCommandLine(&options,
                     argc,
                     argv,
                     UsageMessage());

  verbose = options.verbose_;

  info.real_only_ = options.real_only_;
  info.max_fft_order_ = options.max_fft_order_;
  info.min_fft_order_ = options.min_fft_order_;
  info.do_forward_tests_ = options.do_forward_tests_;
  info.do_inverse_tests_ = options.do_inverse_tests_;
  /* No known failures */
  info.known_failures_ = 0;

  SetThresholds(&info);

  if (verbose > 255)
    DumpOptions(stderr, &options);

  if (options.test_mode_) {
    failed_count = RunAllTests(&info);
  } else {
    TestOneFFT(options.fft_log_size_,
               options.signal_type_,
               options.signal_value_,
               &info,
               "Float FFT");
  }

  FinishedMessage();
  return failed_count > 0 ? 1 : 0;
}

void DumpFFTSpec(OMXFFTSpec_C_FC32* pSpec) {
  ARMsFFTSpec_FC32* p = (ARMsFFTSpec_FC32*) pSpec;
  printf(" N = %d\n", p->N);
  printf(" pBitRev  = %p\n", p->pBitRev);
  printf(" pTwiddle = %p\n", p->pTwiddle);
  printf(" pBuf     = %p\n", p->pBuf);
}

void GenerateSignal(OMX_FC32* x, OMX_FC32* fft, int size, int signal_type,
                    float signal_value) {
  GenerateTestSignalAndFFT((struct ComplexFloat *) x,
                           (struct ComplexFloat *) fft,
                           size,
                           signal_type,
                           signal_value,
                           0);
}

float RunOneForwardTest(int fft_log_size, int signal_type, float signal_value,
                        struct SnrResult* snr) {
  OMX_FC32* x;
  OMX_FC32* y;
  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;

  OMX_FC32* y_true;

  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_C_FC32 * fft_fwd_spec = NULL;
  int fft_size;

  fft_size = 1 << fft_log_size;

  status = omxSP_FFTGetBufSize_C_FC32(fft_log_size, &fft_spec_buffer_size);
  if (verbose > 63) {
    printf("fft_spec_buffer_size = %d\n", fft_spec_buffer_size);
  }

  fft_fwd_spec = (OMXFFTSpec_C_FC32*) malloc(fft_spec_buffer_size);
  status = omxSP_FFTInit_C_FC32(fft_fwd_spec, fft_log_size);
  if (status) {
    fprintf(stderr,
            "Failed to init forward FFT:  status = %d, order %d \n",
            status, fft_log_size);
    exit(1);
  }

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  y_true = (OMX_FC32*) malloc(sizeof(*y_true) * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;

  GenerateSignal(x, y_true, fft_size, signal_type, signal_value);

  if (verbose > 255) {
    printf("&x      = %p\n", (void*) x);
    printf("&y      = %p\n", (void*) y);
    printf("&buffer = %p\n", (void*) ((ARMsFFTSpec_R_FC32*)fft_fwd_spec)->pBuf);
  }

  if (verbose > 63) {
    printf("Signal\n");
    DumpArrayComplexFloat("x", fft_size, x);

    printf("Expected FFT output\n");
    DumpArrayComplexFloat("y", fft_size, y_true);
  }

  status = ForwardFFT(x, y, fft_fwd_spec);

  if (status != OMX_Sts_NoErr) {
    fprintf(stderr, "Forward FFT failed: status = %d\n", status);
    exit(1);
  }

  if (verbose > 63) {
    printf("FFT Output\n");
    DumpArrayComplexFloat("y", fft_size, y);
  }

  CompareComplexFloat(snr, y, y_true, fft_size);

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  free(fft_fwd_spec);

  return snr->complex_snr_;
}

float RunOneInverseTest(int fft_log_size, int signal_type, float signal_value,
                        struct SnrResult* snr) {
  OMX_FC32* x;
  OMX_FC32* y;
  OMX_FC32* z;

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;

  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_C_FC32 * fft_fwd_spec = NULL;
  OMXFFTSpec_C_FC32 * fft_inv_spec = NULL;
  int fft_size;

  fft_size = 1 << fft_log_size;

  status = omxSP_FFTGetBufSize_C_FC32(fft_log_size, &fft_spec_buffer_size);
  if (verbose > 3) {
    printf("fft_spec_buffer_size = %d\n", fft_spec_buffer_size);
  }

  fft_inv_spec = (OMXFFTSpec_C_FC32*)malloc(fft_spec_buffer_size);
  status = omxSP_FFTInit_C_FC32(fft_inv_spec, fft_log_size);
  if (status) {
    fprintf(stderr, "Failed to init backward FFT:  status = %d, order %d\n",
            status, fft_log_size);
    exit(1);
  }

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);
  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  GenerateSignal(x, y, fft_size, signal_type, signal_value);

  if (verbose > 63) {
    printf("Inverse FFT Input Signal\n");
    DumpArrayComplexFloat("x", fft_size, y);

    printf("Expected Inverse FFT output\n");
    DumpArrayComplexFloat("x", fft_size, x);
  }

  status = InverseFFT(y, z, fft_inv_spec);

  if (status != OMX_Sts_NoErr) {
    fprintf(stderr, "Inverse FFT failed: status = %d\n", status);
    exit(1);
  }

  if (verbose > 63) {
    printf("Actual Inverse FFT Output\n");
    DumpArrayComplexFloat("z", fft_size, z);
  }

  CompareComplexFloat(snr, z, x, fft_size);

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  free(fft_inv_spec);

  return snr->complex_snr_;
}
