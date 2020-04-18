/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/test_util.h"
#include "dl/sp/src/test/support/float_rfft_thresholds.h"

static const char* message =
    "Test forward and inverse real floating-point FFT (NEON)\n";

const char* UsageMessage() {
  return message;
}

void FinishedMessage() {
  printf("NEON tests finished.\n");
}

void SetThresholds(struct TestInfo* info) {
  info->forward_threshold_ = FLOAT_RFFT_FORWARD_THRESHOLD_NEON;
  info->inverse_threshold_ = FLOAT_RFFT_INVERSE_THRESHOLD_NEON;
}

OMXResult ForwardRFFT(OMX_F32* x,
                      OMX_F32* y,
                      OMXFFTSpec_R_F32 *fft_fwd_spec) {
  return omxSP_FFTFwd_RToCCS_F32_Sfs(x, y, fft_fwd_spec);
}

OMXResult InverseRFFT(OMX_F32* y,
                      OMX_F32* z,
                      OMXFFTSpec_R_F32 *fft_inv_spec) {
  return omxSP_FFTInv_CCSToR_F32_Sfs(y, z, fft_inv_spec);
}
