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

static const char* message =
    "Test forward and inverse floating-point FFT (Non-NEON)\n";

const char* UsageMessage() {
  return message;
}

void FinishedMessage() {
  printf("Non-NEON tests finished.\n");
}

void SetThresholds(struct TestInfo* info) {
#ifdef BIG_FFT_TABLE
  info->forward_threshold_ = 138.84;
  info->inverse_threshold_ = 137.99;
#else
  info->forward_threshold_ = 139.52;
  info->inverse_threshold_ = 139.21;
#endif
}

OMXResult ForwardFFT(OMX_FC32* x,
                     OMX_FC32* y,
                     OMXFFTSpec_C_FC32 *fft_fwd_spec) {
  return omxSP_FFTFwd_CToC_FC32_Sfs_vfp(x, y, fft_fwd_spec);
}

OMXResult InverseFFT(OMX_FC32* y,
                     OMX_FC32* z,
                     OMXFFTSpec_C_FC32 *fft_inv_spec) {
  return omxSP_FFTInv_CToC_FC32_Sfs_vfp(y, z, fft_inv_spec);
}
