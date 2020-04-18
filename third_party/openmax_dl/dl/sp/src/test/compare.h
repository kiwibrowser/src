/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_ARM_FFT_TEST_COMPARE_H_
#define WEBRTC_ARM_FFT_TEST_COMPARE_H_

#include "dl/api/omxtypes.h"

struct SnrResult {
  /* SNR (in dB) for real component (for complex signals) */
  float real_snr_;

  /* SNR (in dB) for imaginary component (for complex signals) */
  float imag_snr_;

  /* SNR (in dB) for real and complex component (for complex signals) */
  float complex_snr_;
};

/*
 * Compute the SNR between the |actual| and |expected| signals of
 * length |size|.  Three SNRs are computed.  An SNR is computed for
 * just the real component, just the imaginary component, and the
 * complex component.  The computed SNR is in dBs.
 */
void CompareComplex32(struct SnrResult* snr, OMX_SC32* actual,
                      OMX_SC32* expected, int size);
void CompareComplex16(struct SnrResult* snr, OMX_SC16* actual,
                      OMX_SC16* expected, int size);
void CompareReal32(struct SnrResult* snr, OMX_S32* actual,
                   OMX_S32* expected, int size);
void CompareReal16(struct SnrResult* snr, OMX_S16* actual,
                   OMX_S16* expected, int size);
void CompareComplexFloat(struct SnrResult* snr, OMX_FC32* actual,
                         OMX_FC32* expected, int size);
void CompareFloat(struct SnrResult* snr, OMX_F32* actual,
                  OMX_F32* expected, int size);

#endif
