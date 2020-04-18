/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 * Some code in this file was originally from file omxSP_FFTInit_R_S16S32.c
 * which was licensed as follows.
 * It has been relicensed with permission from the copyright holders.
 */

/*
 * OpenMAX DL: v1.0.2
 * Last Modified Revision:
 * Last Modified Date:
 *
 * (c) Copyright 2007-2008 ARM Limited. All Rights Reserved.
 */

#include <stdint.h> 

#include "dl/api/arm/armOMX.h"
#include "dl/api/omxtypes.h"
#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"

/**
 * Function: omxSP_FFTInit_R_S16
 *
 * Description:
 * Initialize the real forward-FFT specification information struct.
 *
 * Remarks:
 * This function is used to initialize the specification structures
 * for functions <ippsFFTFwd_RToCCS_S16_Sfs> and
 * <ippsFFTInv_CCSToR_S16_Sfs>. Memory for *pFFTSpec must be
 * allocated prior to calling this function. The number of bytes
 * required for *pFFTSpec can be determined using
 * <FFTGetBufSize_R_S16>.
 *
 * Parameters:
 * [in]  order       base-2 logarithm of the desired block length;
 *			   valid in the range [1,12].
 * [out] pFFTFwdSpec pointer to the initialized specification structure.
 *
 * Return Value:
 * Standard omxError result. See enumeration for possible result codes.
 *
 */

OMXResult omxSP_FFTInit_R_S16(OMXFFTSpec_R_S16* pFFTSpec, OMX_INT order) {
  OMX_INT i = 0, j = 0;
  OMX_SC16 *pTwiddle = NULL, *pTwiddle1 = NULL, *pTwiddle2 = NULL;
  OMX_SC16 *pTwiddle3 = NULL, *pTwiddle4 = NULL;
  OMX_S16 *pBuf = NULL;
  OMX_U16 *pBitRev = NULL;
  OMX_U32 pTmp = 0;
  OMX_INT Nby2 = 0, N = 0, M = 0, diff = 0, step = 0;
  OMX_S16 x = 0, y = 0, xNeg = 0;
  OMX_S32 xS32 = 0, yS32 = 0;
  ARMsFFTSpec_R_SC16 *pFFTStruct = NULL;

  /* Order zero not allowed */
  if (order == 0) {
    return OMX_Sts_BadArgErr;
  }

  /* Do the initializations */
  pFFTStruct = (ARMsFFTSpec_R_SC16*) pFFTSpec;
  Nby2 = 1 << (order - 1);
  N = Nby2 << 1;
  pBitRev = NULL ;  /* optimized implementations don't use bitreversal */
  pTwiddle = (OMX_SC16*) (sizeof(ARMsFFTSpec_R_SC16) + (OMX_S8*)pFFTSpec);

  /* Align to 32 byte boundary */
  pTmp = ((uintptr_t)pTwiddle)&31;  /* (uintptr_t)pTwiddle % 32 */
  if(pTmp != 0) {
    pTwiddle = (OMX_SC16*) ((OMX_S8*)pTwiddle + (32 - pTmp));
  }

  pBuf = (OMX_S16*) (sizeof(OMX_SC16) * (5 * N / 8) + (OMX_S8*)pTwiddle);

  /* Align to 32 byte boundary */
  pTmp = ((OMX_U32)pBuf)&31;                 /* (OMX_U32)pBuf % 32 */
  if(pTmp != 0) {
    pBuf = (OMX_S16*)((OMX_S8*)pBuf + (32 - pTmp));
  }

  /*
   * Filling Twiddle factors : exp^(-j*2*PI*k/ (N/2) ) ; k=0,1,2,...,3/4(N/2).
   * N/2 point complex FFT is used to compute N point real FFT.
   * The original twiddle table "armSP_FFT_S32TwiddleTable" is of size
   * (MaxSize/8 + 1). Rest of the values i.e., up to MaxSize are calculated
   * using the symmetries of sin and cos.
   * The max size of the twiddle table needed is 3/4(N/2) for a radix-4 stage.
   *
   * W = (-2 * PI) / N
   * N = 1 << order
   * W = -PI >> (order - 1)
   * 
   * Note we use S32 twiddle factor table and round the values to 16 bits.
   */

  M = Nby2 >> 3;
  diff = 12 - (order - 1);
  step = 1 << diff;  /* Step into the twiddle table for the current order */

  xS32 = armSP_FFT_S32TwiddleTable[0];
  yS32 = armSP_FFT_S32TwiddleTable[1];
  x = (xS32 + 0x8000) >> 16;
  y = (yS32 + 0x8000) >> 16;
  xNeg = 0x7FFF;

  if((order-1) >= 3) {
    /* i = 0 case */
    pTwiddle[0].Re = x;
    pTwiddle[0].Im = y;
    pTwiddle[2 * M].Re = -y;
    pTwiddle[2 * M].Im = xNeg;
    pTwiddle[4 * M].Re = xNeg;
    pTwiddle[4 * M].Im = y;

    for (i=1; i<=M; i++){
      OMX_S16 x_neg = 0, y_neg = 0;
      j = i * step;

      xS32 = armSP_FFT_S32TwiddleTable[2 * j];
      yS32 = armSP_FFT_S32TwiddleTable[2 * j + 1];
      x = (xS32 + 0x8000) >> 16;
      y = (yS32 + 0x8000) >> 16;
      /* |x_neg = -x| doesn't work when x is 0x8000. */
      x_neg = (-(xS32 + 0x8000)) >> 16;
      y_neg = (-(yS32 + 0x8000)) >> 16;

      pTwiddle[i].Re = x;
      pTwiddle[i].Im = y;
      pTwiddle[2 * M - i].Re = y_neg;
      pTwiddle[2 * M - i].Im = x_neg;
      pTwiddle[2 * M + i].Re = y;
      pTwiddle[2 * M + i].Im = x_neg;
      pTwiddle[4 * M - i].Re = x_neg;
      pTwiddle[4 * M - i].Im = y;
      pTwiddle[4 * M + i].Re = x_neg;
      pTwiddle[4 * M + i].Im = y_neg;
      pTwiddle[6 * M - i].Re = y;
      pTwiddle[6 * M - i].Im = x;
    }
  }
  else {
    if ((order - 1) == 2) {
      pTwiddle[0].Re = x;
      pTwiddle[0].Im = y;
      pTwiddle[1].Re = -y;
      pTwiddle[1].Im = xNeg;
      pTwiddle[2].Re = xNeg;
      pTwiddle[2].Im = y;
    }
    if ((order-1) == 1) {
      pTwiddle[0].Re = x;
      pTwiddle[0].Im = y;
    }
  }

  /*
   * Now fill the last N/4 values : exp^(-j*2*PI*k/N);  k=1,3,5,...,N/2-1.
   * These are used for the final twiddle fix-up for converting complex to
   * real FFT.
   */

  M = N >> 3;
  diff = 12 - order;
  step = 1 << diff;

  pTwiddle1 = pTwiddle + 3 * N / 8;
  pTwiddle4 = pTwiddle1 + (N / 4 - 1);
  pTwiddle3 = pTwiddle1 + N / 8;
  pTwiddle2 = pTwiddle1 + (N / 8 - 1);

  xS32 = armSP_FFT_S32TwiddleTable[0];
  yS32 = armSP_FFT_S32TwiddleTable[1];
  x = (xS32 + 0x8000) >> 16;
  y = (yS32 + 0x8000) >> 16;
  xNeg = 0x7FFF;

  if((order) >= 3) {
    for (i = 1; i <= M; i += 2 ) {
      OMX_S16 x_neg = 0, y_neg = 0;

      j = i*step;

      xS32 = armSP_FFT_S32TwiddleTable[2 * j];
      yS32 = armSP_FFT_S32TwiddleTable[2 * j + 1];
      x = (xS32 + 0x8000) >> 16;
      y = (yS32 + 0x8000) >> 16;
      /* |x_neg = -x| doesn't work when x is 0x8000. */
      x_neg = (-(xS32 + 0x8000)) >> 16;
      y_neg = (-(yS32 + 0x8000)) >> 16;

      pTwiddle1[0].Re = x;
      pTwiddle1[0].Im = y;
      pTwiddle1 += 1;
      pTwiddle2[0].Re = y_neg;
      pTwiddle2[0].Im = x_neg;
      pTwiddle2 -= 1;
      pTwiddle3[0].Re = y;
      pTwiddle3[0].Im = x_neg;
      pTwiddle3 += 1;
      pTwiddle4[0].Re = x_neg;
      pTwiddle4[0].Im = y;
      pTwiddle4 -= 1;
    }
  }
  else {
    if (order == 2) {
      pTwiddle1[0].Re = -y;
      pTwiddle1[0].Im = xNeg;
    }
  }

  /* Update the structure */
  pFFTStruct->N = N;
  pFFTStruct->pTwiddle = pTwiddle;
  pFFTStruct->pBitRev = pBitRev;
  pFFTStruct->pBuf = pBuf;

  return OMX_Sts_NoErr;
}
/*****************************************************************************
 *                              END OF FILE
 *****************************************************************************/

