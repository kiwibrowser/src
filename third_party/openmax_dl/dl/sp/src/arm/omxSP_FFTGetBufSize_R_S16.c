/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 * Some code in this file was originally from file omxSP_FFTGetBufSize_R_S32.c
 * which was licensed as follows.
 * It has been relicensed with permission from the copyright holders.
 */

/*
 * OpenMAX DL: v1.0.2
 * Last Modified Revision:
 * Last Modified Date:
 */

#include "dl/api/arm/armOMX.h"
#include "dl/api/omxtypes.h"
#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"

/**
 * Function: omxSP_FFTGetBufSize_R_S16
 *
 * Description:
 * Computes the size of the specification structure required for the length
 * 2^order real FFT and IFFT functions.
 *
 * Remarks:
 * This function is used in conjunction with the 16-bit functions
 * <FFTFwd_RToCCS_S16_Sfs> and <FFTInv_CCSToR_S16_Sfs>.
 *
 * Parameters:
 * [in]  order       base-2 logarithm of the length; valid in the range
 *			   [1,12].
 * [out] pSize	   pointer to the number of bytes required for the
 *			   specification structure.
 *
 * Return Value:
 * Standard omxError result. See enumeration for possible result codes.
 *
 */

OMXResult omxSP_FFTGetBufSize_R_S16(OMX_INT order, OMX_INT *pSize) {
  OMX_INT     NBy2,N,twiddleSize;

  /* Order zero not allowed */
  if (order == 0) {
    return OMX_Sts_BadArgErr;
  }

  NBy2 = 1 << (order - 1);
  N = NBy2 << 1;
  twiddleSize = 5 * N / 8;  /* 3 / 4 (N / 2) + N / 4 */

  /* 2 pointers to store bitreversed array and twiddle factor array */
  *pSize = sizeof(ARMsFFTSpec_R_SC16)
           /* Twiddle factors  */
           + sizeof(OMX_SC16) * twiddleSize
           /* Ping Pong buffer for doing the N/2 point complex FFT; */
           /* extra size 'N' as a temporary buf for FFTInv_CCSToR_S16_Sfs */
           + sizeof(OMX_S16) * (N << 1)
           /* Extra bytes to get 32 byte alignment of ptwiddle and pBuf */
           + 62 ;


  return OMX_Sts_NoErr;
}

/*****************************************************************************
 *                              END OF FILE
 *****************************************************************************/

