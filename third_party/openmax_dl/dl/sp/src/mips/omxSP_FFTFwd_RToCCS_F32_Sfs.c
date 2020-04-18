/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#include <stdint.h>

#include "dl/api/omxtypes.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/api/mipsSP.h"

extern OMXResult mips_FFTFwd_RToCCS_F32_complex(
    const OMX_F32* pSrc,
    OMX_F32* pDst,
    const MIPSFFTSpec_R_FC32* pFFTSpec);

extern OMXResult mips_FFTFwd_RToCCS_F32_real(
    const OMX_F32* pSrc,
    OMX_F32* pDst,
    const MIPSFFTSpec_R_FC32* pFFTSpec);

OMXResult omxSP_FFTFwd_RToCCS_F32_Sfs(const OMX_F32* pSrc,
                                      OMX_F32* pDst,
                                      const OMXFFTSpec_R_F32* pFFTSpec) {
  const MIPSFFTSpec_R_FC32* pFFTStruct = (const MIPSFFTSpec_R_FC32*)pFFTSpec;

  /* Buffers must be 32 bytes aligned. */
  if (!pSrc || !pDst || ((uintptr_t)pSrc & 31) || ((uintptr_t)pDst & 31) ||
      !pFFTSpec)
    return OMX_Sts_BadArgErr;

  /* Check if the structure is initialized correctly. */
  if (!pFFTStruct->pBitRev || !pFFTStruct->pBitRevInv || !pFFTStruct->pOffset ||
      !pFFTStruct->pTwiddle || !pFFTStruct->pBuf || (pFFTStruct->order < 1) ||
      (pFFTStruct->order > TWIDDLE_TABLE_ORDER))
    return OMX_Sts_BadArgErr;

  /* For order larger than 4, compute Real FFT as Complex FFT of (order - 1). */
  if (pFFTStruct->order > 4)
    return mips_FFTFwd_RToCCS_F32_complex(pSrc, pDst, pFFTStruct);

  /* Special case for order == 1. */
  if (pFFTStruct->order == 1) {
    pDst[0] = (pSrc[0] + pSrc[1]);
    pDst[1] = 0.0f;
    pDst[2] = (pSrc[0] - pSrc[1]);
    pDst[3] = 0.0f;
    return OMX_Sts_NoErr;
  }

  /* Other short FFTs. */
  return mips_FFTFwd_RToCCS_F32_real(pSrc, pDst, pFFTStruct);
}
