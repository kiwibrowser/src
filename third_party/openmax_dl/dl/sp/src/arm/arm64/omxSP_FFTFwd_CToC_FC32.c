/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "dl/api/omxtypes.h"
#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"

extern void armSP_FFTFwd_CToC_FC32_Radix2_fs_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTFwd_CToC_FC32_Radix2_ls_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTFwd_CToC_FC32_Radix2_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTFwd_CToC_FC32_Radix4_fs_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTFwd_CToC_FC32_Radix4_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTFwd_CToC_FC32_Radix4_ls_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTFwd_CToC_FC32_Radix8_fs_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

/**
 * Function:  omxSP_FFTFwd_CToC_FC32_Sfs   (2.2.4.2.2)
 *
 * Description:
 * Compute an FFT for a complex signal of length of 2^order, 
 * where 0 <= order <= 15. 
 * Transform length is determined by the specification structure, which 
 * must be initialized prior to calling the FFT function using the appropriate 
 * helper, i.e., <FFTInit_C_sc32> or <FFTInit_C_SC16>. The relationship 
 * between the input and output sequences can be expressed in terms of the 
 * DFT, i.e., 
 *
 *      X[k] = SUM[n=0...N-1]x[n].e^(-jnk.2.pi/N)
 *      k = 0,1,2,..., N-1
 *      N = 2^order
 *
 * Input Arguments:
 *   pSrc - pointer to the input signal, a complex-valued vector of length
 *            2^order; must be aligned on a 32 byte boundary. 
 *   pFFTSpec - pointer to the preallocated and initialized specification 
 *            structure 
 *
 * Output Arguments:
 *   pDst - pointer to the complex-valued output vector, of length 2^order;
 *            must be aligned on an 32-byte boundary. 
 *
 * Return Value:
 *    
 *    OMX_Sts_NoErr - no error 
 *    OMX_Sts_BadArgErr - returned if one or more of the following conditions 
 *              is true: 
 *    -   one or more of the following pointers is NULL: pSrc, pDst, or 
 *              pFFTSpec. 
 *    -    pSrc or pDst is not 32-byte aligned 
 *
 */

OMXResult omxSP_FFTFwd_CToC_FC32_Sfs(const OMX_FC32* pSrc,
                                     OMX_FC32* pDst,
                                     const OMXFFTSpec_C_FC32* pFFTSpec) {
  ARMsFFTSpec_FC32* spec = (ARMsFFTSpec_FC32*)pFFTSpec;
  int order;
  long subFFTSize;
  long subFFTNum;
  OMX_FC32* pTwiddle;
  OMX_FC32* pOut;

  /*
   * Check args are not NULL and the source and destination pointers
   * are properly aligned.
   */
  if (!validateParametersFC32(pSrc, pDst, spec))
    return OMX_Sts_BadArgErr;

  order = fastlog2(spec->N);

  subFFTSize = 1;
  subFFTNum = spec->N;
  pTwiddle = spec->pTwiddle;
  pOut = spec->pBuf;

  if (order > 3) {
    OMX_FC32* argDst;

    /*
     * Set up argDst and pOut appropriately so that pOut = pDst for
     * the very last FFT stage.
     */
    if ((order & 2) == 0) {
      argDst = pOut;
      pOut = pDst;
    } else {
      argDst = pDst;
    }

    /*
     * Odd order uses a radix 8 first stage; even order, a radix 4
     * first stage.
     */
    if (order & 1) {
      armSP_FFTFwd_CToC_FC32_Radix8_fs_OutOfPlace(
          pSrc, argDst, pTwiddle, &subFFTNum, &subFFTSize);
    } else {
      armSP_FFTFwd_CToC_FC32_Radix4_fs_OutOfPlace(
          pSrc, argDst, pTwiddle, &subFFTNum, &subFFTSize);
    }

    /*
     * Now use radix 4 stages to finish rest of the FFT
     */
    if (subFFTNum >= 4) {
      while (subFFTNum > 4) {
        OMX_FC32* tmp;

        armSP_FFTFwd_CToC_FC32_Radix4_OutOfPlace(
            argDst, pOut, pTwiddle, &subFFTNum, &subFFTSize);
        /*
         * Swap argDst and pOut
         */
        tmp = pOut;
        pOut = argDst;
        argDst = tmp;
      }

      armSP_FFTFwd_CToC_FC32_Radix4_ls_OutOfPlace(
          argDst, pOut, pTwiddle, &subFFTNum, &subFFTSize);
    }
  } else if (order == 3) {
    armSP_FFTFwd_CToC_FC32_Radix2_fs_OutOfPlace(
        pSrc, pDst, pTwiddle, &subFFTNum, &subFFTSize);
    armSP_FFTFwd_CToC_FC32_Radix2_OutOfPlace(
        pDst, pOut, pTwiddle, &subFFTNum, &subFFTSize);
    armSP_FFTFwd_CToC_FC32_Radix2_ls_OutOfPlace(
        pOut, pDst, pTwiddle, &subFFTNum, &subFFTSize);
  } else if (order == 2) {
    armSP_FFTFwd_CToC_FC32_Radix2_fs_OutOfPlace(
        pSrc, pOut, pTwiddle, &subFFTNum, &subFFTSize);
    armSP_FFTFwd_CToC_FC32_Radix2_ls_OutOfPlace(
        pOut, pDst, pTwiddle, &subFFTNum, &subFFTSize);
  } else {
    /* Order = 1 */
    armSP_FFTFwd_CToC_FC32_Radix2_fs_OutOfPlace(
        pSrc, pDst, pTwiddle, &subFFTNum, &subFFTSize);
  }

  return OMX_Sts_NoErr;
}
