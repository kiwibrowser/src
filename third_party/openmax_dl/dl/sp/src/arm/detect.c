/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#include <cpu-features.h>

#include "android/log.h"
#include "dl/sp/api/omxSP.h"

int omxSP_HasArmNeon() {
  return (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0;
}

static void SetFFTRoutines() {
  /*
   * Choose the correct (NEON or non-NEON) routines for both the
   * forward and inverse FFTs
   */
  if (omxSP_HasArmNeon()) {
    __android_log_print(ANDROID_LOG_INFO, "OpenMAX DL FFT",
                        "Using NEON FFT");
    omxSP_FFTFwd_RToCCS_F32 = omxSP_FFTFwd_RToCCS_F32_Sfs;
    omxSP_FFTInv_CCSToR_F32 = omxSP_FFTInv_CCSToR_F32_Sfs;
  } else {
    __android_log_print(ANDROID_LOG_INFO, "OpenMAX DL FFT",
                        "Using non-NEON FFT");
    omxSP_FFTFwd_RToCCS_F32 = omxSP_FFTFwd_RToCCS_F32_Sfs_vfp;
    omxSP_FFTInv_CCSToR_F32 = omxSP_FFTInv_CCSToR_F32_Sfs_vfp;
  }
}

/*
 * FIXME: It would be beneficial to use the GCC ifunc attribute to
 * select the appropriate function at load time. This is apparently
 * not supported on Android at this time. (Compiler warning that the
 * ifunc attribute is ignored.)
 */

/*
 * Forward FFT.  Detect if NEON is supported and update function
 * pointers to the correct routines for both the forward and inverse
 * FFTs.  Then run the forward FFT routine.
 */
static OMXResult DetectForwardRealFFT(
    const OMX_F32* pSrc,
    OMX_F32* pDst,
    const OMXFFTSpec_R_F32* pFFTSpec) {
  SetFFTRoutines();
  return omxSP_FFTFwd_RToCCS_F32(pSrc, pDst, pFFTSpec);
}

/*
 * Inverse FFT.  Detect if NEON is supported and update function
 * pointers to the correct routines for both the forward and inverse
 * FFTs.  Then run the inverse FFT routine.
 */
static OMXResult DetectInverseRealFFT(
    const OMX_F32* pSrc,
    OMX_F32* pDst,
    const OMXFFTSpec_R_F32* pFFTSpec) {
  SetFFTRoutines();
  return omxSP_FFTInv_CCSToR_F32(pSrc, pDst, pFFTSpec);
}

/*
 * Implementation of the forward and inverse real float FFT.
 * Initialize to detection routine which will update the pointer to
 * the correct routine and then call the correct one.
 */
OMXResult (*omxSP_FFTFwd_RToCCS_F32)(
    const OMX_F32* pSrc,
    OMX_F32* pDst,
    const OMXFFTSpec_R_F32* pFFTSpec) = DetectForwardRealFFT;

OMXResult (*omxSP_FFTInv_CCSToR_F32)(
    const OMX_F32* pSrc,
    OMX_F32* pDst,
    const OMXFFTSpec_R_F32* pFFTSpec) = DetectInverseRealFFT;
