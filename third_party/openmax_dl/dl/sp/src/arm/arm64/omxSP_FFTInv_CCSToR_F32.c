/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <arm_neon.h>

#include "dl/api/omxtypes.h"
#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"

extern void armSP_FFTInv_CToC_FC32_Radix2_fs_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTInv_CToC_FC32_Radix2_ls_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTInv_CToC_FC32_Radix2_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTInv_CToC_FC32_Radix4_fs_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTInv_CToC_FC32_Radix4_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTInv_CToC_FC32_Radix4_ls_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTInv_CToC_FC32_Radix8_fs_OutOfPlace(
    const OMX_FC32* pSrc,
    OMX_FC32* pDst,
    OMX_FC32* pTwiddle,
    long* subFFTNum,
    long* subFFTSize);

extern void armSP_FFTInv_CCSToR_F32_preTwiddleRadix2(
    const OMX_F32* pSrc,
    const OMX_FC32* pTwiddle,
    OMX_F32* pBuf,
    long N);

/*
 * Scale FFT data by 1/|length|. |length| must be a power of two
 */
static inline void ScaleRFFTData(OMX_F32* fftData, unsigned length) {
  float32_t* data = (float32_t*)fftData;
  float32_t scale = 2.0f / length;

  if (length >= 4) {
    /*
     * Do 4 float elements at a time because |length| is always a
     * multiple of 4 when |length| >= 4.
     *
     * TODO(rtoy): Figure out how to process 8 elements at a time
     * using intrinsics or replace this with inline assembly.
     */
    do {
      float32x4_t x = vld1q_f32(data);

      length -= 4;
      x = vmulq_n_f32(x, scale);
      vst1q_f32(data, x);
      data += 4;
    } while (length > 0);
  } else if (length == 2) {
    float32x2_t x = vld1_f32(data);
    x = vmul_n_f32(x, scale);
    vst1_f32(data, x);
  } else {
    fftData[0] *= scale;
  }
}

/**
 * Function:  omxSP_FFTInv_CCSToR_F32_Sfs
 *
 * Description:
 * These functions compute the inverse FFT for a conjugate-symmetric input 
 * sequence.  Transform length is determined by the specification structure, 
 * which must be initialized prior to calling the FFT function using 
 * <FFTInit_R_F32>. For a transform of length M, the input sequence is 
 * represented using a packed CCS vector of length M+2, and is organized 
 * as follows: 
 *
 *   Index:   0  1  2    3    4    5    . . .  M-2       M-1      M      M+1 
 *   Comp:  R[0] 0  R[1] I[1] R[2] I[2] . . .  R[M/2-1]  I[M/2-1] R[M/2] 0 
 *
 * where R[n] and I[n], respectively, denote the real and imaginary
 * components for FFT bin n. Bins are numbered from 0 to M/2, where M
 * is the FFT length.  Bin index 0 corresponds to the DC component,
 * and bin index M/2 corresponds to the foldover frequency.
 *
 * Input Arguments:
 *   pSrc - pointer to the complex-valued input sequence represented
 *          using CCS format, of length (2^order) + 2; must be aligned on a
 *          32-byte boundary.
 *   pFFTSpec - pointer to the preallocated and initialized
 *              specification structure
 *
 * Output Arguments:
 *   pDst - pointer to the real-valued output sequence, of length
 *          2^order ; must be aligned on a 32-byte boundary.
 *
 * Return Value:
 *    
 *    OMX_Sts_NoErr - no error 

 *    OMX_Sts_BadArgErr - bad arguments if one or more of the
 *      following is true:
 *    -    pSrc, pDst, or pFFTSpec is NULL 
 *    -    pSrc or pDst is not aligned on a 32-byte boundary 
 *
 */
OMXResult omxSP_FFTInv_CCSToR_F32_Sfs(
    const OMX_F32* pSrc,
    OMX_F32* pDst,
    const OMXFFTSpec_R_F32* pFFTSpec) {
  ARMsFFTSpec_R_FC32* spec = (ARMsFFTSpec_R_FC32*)pFFTSpec;
  int order;
  long subFFTSize;
  long subFFTNum;
  OMX_FC32* pTwiddle;
  OMX_FC32* pOut;
  OMX_FC32* pComplexSrc;
  OMX_FC32* pComplexDst = (OMX_FC32*) pDst;

  /*
   * Check args are not NULL and the source and destination pointers
   * are properly aligned.
   */
  if (!validateParametersF32(pSrc, pDst, spec))
    return OMX_Sts_BadArgErr;

  /*
   * Preprocess the input before calling the complex inverse FFT. The
   * result is actually stored in the second half of the temp buffer
   * in pFFTSpec.
   */
  if (spec->N > 1)
    armSP_FFTInv_CCSToR_F32_preTwiddleRadix2(
        pSrc, spec->pTwiddle, spec->pBuf, spec->N);
  
  /*
   * Do a complex inverse FFT of half size.
   */
  order = fastlog2(spec->N) - 1;

  subFFTSize = 1;
  subFFTNum = spec->N >> 1;
  pTwiddle = spec->pTwiddle;
  /*
   * The pBuf is split in half. The first half is the temp buffer. The
   * second half holds the source data that was placed there by
   * armSP_FFTInv_CCSToR_F32_preTwiddleRadix2_unsafe.
   */
  pOut = (OMX_FC32*) spec->pBuf;
  pComplexSrc = pOut + (1 << order);


  if (order > 3) {
    OMX_FC32* argDst;

    /*
     * Set up argDst and pOut appropriately so that pOut = pDst for
     * the very last FFT stage.
     */
    if ((order & 2) == 0) {
      argDst = pOut;
      pOut = pComplexDst;
    } else {
      argDst = pComplexDst;
    }

    /*
     * Odd order uses a radix 8 first stage; even order, a radix 4
     * first stage.
     */
    if (order & 1) {
      armSP_FFTInv_CToC_FC32_Radix8_fs_OutOfPlace(
          pComplexSrc, argDst, pTwiddle, &subFFTNum, &subFFTSize);
    } else {
      armSP_FFTInv_CToC_FC32_Radix4_fs_OutOfPlace(
          pComplexSrc, argDst, pTwiddle, &subFFTNum, &subFFTSize);
    }

    /*
     * Now use radix 4 stages to finish rest of the FFT
     */
    if (subFFTNum >= 4) {
      while (subFFTNum > 4) {
        OMX_FC32* tmp;

        armSP_FFTInv_CToC_FC32_Radix4_OutOfPlace(
            argDst, pOut, pTwiddle, &subFFTNum, &subFFTSize);
        /*
         * Swap argDst and pOut
         */
        tmp = pOut;
        pOut = argDst;
        argDst = tmp;
      }

      armSP_FFTInv_CToC_FC32_Radix4_ls_OutOfPlace(
          argDst, pOut, pTwiddle, &subFFTNum, &subFFTSize);
    }
  } else if (order == 3) {
    armSP_FFTInv_CToC_FC32_Radix2_fs_OutOfPlace(
        pComplexSrc, pComplexDst, pTwiddle, &subFFTNum, &subFFTSize);
    armSP_FFTInv_CToC_FC32_Radix2_OutOfPlace(
        pComplexDst, pOut, pTwiddle, &subFFTNum, &subFFTSize);
    armSP_FFTInv_CToC_FC32_Radix2_ls_OutOfPlace(
        pOut, pComplexDst, pTwiddle, &subFFTNum, &subFFTSize);
  } else if (order == 2) {
    armSP_FFTInv_CToC_FC32_Radix2_fs_OutOfPlace(
        pComplexSrc, pOut, pTwiddle, &subFFTNum, &subFFTSize);
    armSP_FFTInv_CToC_FC32_Radix2_ls_OutOfPlace(
        pOut, pComplexDst, pTwiddle, &subFFTNum, &subFFTSize);
  } else if (order == 1) {
    armSP_FFTInv_CToC_FC32_Radix2_fs_OutOfPlace(
        pComplexSrc, pComplexDst, pTwiddle, &subFFTNum, &subFFTSize);
  } else {
    /* Order = 0 */
    *pComplexDst = *pComplexSrc;
  }

  ScaleRFFTData(pDst, spec->N);
  return OMX_Sts_NoErr;
}

