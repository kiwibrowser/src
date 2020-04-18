/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/test_util.h"

static const char* message =
    "Test forward and inverse floating-point FFT"
#if defined(__aarch64__)
    " (ARM64)\n"
#else
    " (NEON)\n"
#endif
    ;

const char* UsageMessage() {
  return message;
}

#if defined(__aarch64__)
#define FINISHED_MESSAGE "ARM64 tests finished.\n"
#else
#define FINISHED_MESSAGE "NEON tests finished.\n"
#endif

void FinishedMessage() {
  printf(FINISHED_MESSAGE);
}

void SetThresholds(struct TestInfo* info) {
#if defined(__arm__)
#ifdef BIG_FFT_TABLE
  info->forward_threshold_ = 138.81;
  info->inverse_threshold_ = 137.81;
#else
  info->forward_threshold_ = 138.81;
  info->inverse_threshold_ = 138.81;
#endif
#else
#ifdef BIG_FFT_TABLE
  info->forward_threshold_ = 138.96;
  info->inverse_threshold_ = 138.96;
#else
  info->forward_threshold_ = 138.96;
  info->inverse_threshold_ = 138.96;
#endif
#endif
}

OMXResult ForwardFFT(OMX_FC32* x,
                     OMX_FC32* y,
                     OMXFFTSpec_C_FC32 *fft_fwd_spec) {
  return omxSP_FFTFwd_CToC_FC32_Sfs(x, y, fft_fwd_spec);
}

OMXResult InverseFFT(OMX_FC32* y,
                     OMX_FC32* z,
                     OMXFFTSpec_C_FC32 *fft_inv_spec) {
  return omxSP_FFTInv_CToC_FC32_Sfs(y, z, fft_inv_spec);
}
