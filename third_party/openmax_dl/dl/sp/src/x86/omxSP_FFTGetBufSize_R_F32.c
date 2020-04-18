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

#include "dl/api/omxtypes.h"
#include "dl/sp/api/x86SP.h"
#include "dl/sp/api/omxSP.h"

/**
 * Function: omxSP_FFTGetBufSize_R_F32
 *
 * Description:
 * Computes the size of the specification structure required for the length
 * 2^order real FFT and IFFT functions.
 *
 * Remarks:
 * This function is used in conjunction with the 32-bit functions
 * <FFTFwd_RToCCS_F32_Sfs> and <FFTInv_CCSToR_F32_Sfs>.
 *
 * Parameters:
 * [in]  order       base-2 logarithm of the length; valid in the range
 *                    [1,12]. ([1,15] if BIG_FFT_TABLE is defined.)
 * [out] pSize	   pointer to the number of bytes required for the
 *			   specification structure.
 *
 * Return Value:
 * Standard omxError result. See enumeration for possible result codes.
 *
 */

OMXResult omxSP_FFTGetBufSize_R_F32(OMX_INT order, OMX_INT *pSize) {
  OMX_INT n_by_2;
  OMX_INT n;

  if (!pSize || (order < 1) || (order > TWIDDLE_TABLE_ORDER))
    return OMX_Sts_BadArgErr;

  n_by_2 = 1 << (order - 1);
  n = n_by_2 << 1;

  *pSize = sizeof(X86FFTSpec_R_FC32) +
           // Twiddle factors.
           sizeof(OMX_F32) * (n << 1) +
           // Ping Pong buffer for doing the n/2 point complex FFT.
           // pBuf1
           sizeof(OMX_F32) * n + 4 +
           // pBuf2
           sizeof(OMX_F32) * n + 4 +
           // Extra bytes to get 32 byte alignment of ptwiddle, pBuf1
           62;

  return OMX_Sts_NoErr;
}
