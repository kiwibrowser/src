/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 *  This is a modification of omxSP_FFTInit_R_S32.c to support float
 *  instead of S32.
 */

#include <stdint.h>

#include "dl/api/omxtypes.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/api/x86SP.h"

/**
 * Function: omxSP_FFTInit_R_F32
 *
 * Description:
 * Initialize the real forward-FFT specification information struct.
 *
 * Remarks:
 * This function is used to initialize the specification structures
 * for functions |omxSP_FFTFwd_RToCCS_F32_Sfs| and
 * |omxSP_FFTInv_CCSToR_F32_Sfs|. Memory for *pFFTSpec must be
 * allocated prior to calling this function. The number of bytes
 * required for *pFFTSpec can be determined using
 * |omxSP_FFTGetBufSize_R_F32|.
 *
 * Parameters:
 * [in]  order       base-2 logarithm of the desired block length;
 *                         valid in the range [1,12].  ([1,15] if
 *                         BIG_FFT_TABLE is defined.)
 * [out] pFFTFwdSpec pointer to the initialized specification structure.
 *
 * Return Value:
 * Standard omxError result. See enumeration for possible result codes.
 *
 */

OMXResult omxSP_FFTInit_R_F32(OMXFFTSpec_R_F32 *pFFTSpec, OMX_INT order)
{
  OMX_F32 *pTwiddle;
  OMX_F32 *pBuf;
  OMX_INT i;
  OMX_INT j;
  OMX_INT N;
  OMX_INT NBy2;
  OMX_INT NBy4;
  OMX_INT diff;
  OMX_U32 pTmp;
  X86FFTSpec_R_FC32  *pFFTStruct = (X86FFTSpec_R_FC32 *) pFFTSpec;
  OMX_F32 real;
  OMX_F32 imag;

  if (!pFFTSpec || (order < 1) || (order > TWIDDLE_TABLE_ORDER))
    return OMX_Sts_BadArgErr;

  N = 1 << order;
  NBy2 = N >> 1;

  pTwiddle = (OMX_F32*) (sizeof(X86FFTSpec_R_FC32) + (OMX_S8*) pFFTSpec);

  // Align to 32 byte boundary.
  pTmp = ((uintptr_t)pTwiddle) & 31;
  if (pTmp)
    pTwiddle = (OMX_F32*) ((OMX_S8*)pTwiddle + (32 - pTmp));

  pBuf = (OMX_F32*) (sizeof(OMX_F32) * (N << 1) + (OMX_S8*) pTwiddle);

  // Align to 32 byte boundary.
  pTmp = ((uintptr_t)pBuf) & 31;
  if (pTmp)
    pBuf = (OMX_F32*) ((OMX_S8*)pBuf + (32 - pTmp));

  // Calculating Twiddle Factors.
  diff = 1 << (TWIDDLE_TABLE_ORDER - order + 1);

  // For SSE optimization, using twiddle with split format by which the real and
  // imag data are stored into first and last halves of the buffer separately
  // The negatives are moved when generating pTwiddle table.
  if (order > 1) {
    NBy4 = N >> 2;
    for (i = 0, j = 0; i <= NBy4 >> 1; ++i, j += diff) {
      real = armSP_FFT_F32TwiddleTable[j];
      imag = armSP_FFT_F32TwiddleTable[j + 1];

      pTwiddle[i] = -real;
      pTwiddle[i + N] = -imag;

      pTwiddle[NBy4 - i] = imag;
      pTwiddle[NBy4 - i + N] = real;

      pTwiddle[NBy4 + i] = -imag;
      pTwiddle[NBy4 + i + N] = real;

      pTwiddle[NBy2 - i] = real;
      pTwiddle[NBy2 - i + N] = -imag;

      pTwiddle[NBy2 + i] = real;
      pTwiddle[NBy2 + i + N] = imag;

      pTwiddle[NBy4 * 3 - i] = -imag;
      pTwiddle[NBy4 * 3 - i + N] = -real;

      pTwiddle[NBy4 * 3 + i] = imag;
      pTwiddle[NBy4 * 3 + i + N] = -real;

      pTwiddle[N - i - 1] = -real;
      pTwiddle[(N << 1) - i - 1] = imag;
    }
  } else {
    pTwiddle[0] = armSP_FFT_F32TwiddleTable[0];
    pTwiddle[2] = armSP_FFT_F32TwiddleTable[1];
    pTwiddle[1] = -pTwiddle[0];
    pTwiddle[3] = pTwiddle[2];
  }
  pFFTStruct->N = N;
  pFFTStruct->pTwiddle = pTwiddle;
  pFFTStruct->pBuf1 = pBuf;
  pFFTStruct->pBuf2 = pBuf + N + 4;

  return OMX_Sts_NoErr;
}
