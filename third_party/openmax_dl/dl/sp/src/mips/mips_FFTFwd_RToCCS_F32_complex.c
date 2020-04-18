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
#include "dl/sp/api/mipsSP.h"

/*
 * Forward real FFT for FFT sizes larger than 16. Computed using the complex
 * FFT for half the size.
 */
OMXResult mips_FFTFwd_RToCCS_F32_complex(const OMX_F32* pSrc,
                                         OMX_F32* pDst,
                                         const MIPSFFTSpec_R_FC32* pFFTSpec) {
  OMX_U32 n1_4, num_transforms, step;
  const OMX_F32* w_re_ptr;
  const OMX_F32* w_im_ptr;
  OMX_U32 fft_size = 1 << pFFTSpec->order;
  OMX_FC32* p_dst = (OMX_FC32*)pDst;
  OMX_FC32* p_buf = (OMX_FC32*)pFFTSpec->pBuf;
  OMX_F32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
  OMX_F32 w_re, w_im;

  /*
   * Loop performing sub-transforms of size 4,
   * which contain 2 butterfly operations.
   */
  num_transforms = (SUBTRANSFORM_CONST >> (17 - pFFTSpec->order)) | 1;
  for (uint32_t n = 0; n < num_transforms; ++n) {
    /*
     * n is in the range (0 .. num_transforms - 1).
     * The size of the pFFTSpec->pOffset is (((SUBTRANSFORM_CONST >>
     * (16 - TWIDDLE_TABLE_ORDER)) | 1)).
     */
    OMX_U32 offset = pFFTSpec->pOffset[n] << 2;
    /*
     * Offset takes it's value from pFFTSpec->pOffset table which is initialized
     * in the omxSP_FFTInit_R_F32 function, and is constant afterwards.
     */
    OMX_U16* p_bitrev = pFFTSpec->pBitRev + offset;
    OMX_FC32* p_tmp = p_buf + offset;
    /* Treating the input as a complex vector. */
    const OMX_FC32* p_src = (const OMX_FC32*)pSrc;

    tmp1 = p_src[p_bitrev[0]].Re + p_src[p_bitrev[1]].Re;
    tmp2 = p_src[p_bitrev[2]].Re + p_src[p_bitrev[3]].Re;
    tmp3 = p_src[p_bitrev[0]].Im + p_src[p_bitrev[1]].Im;
    tmp4 = p_src[p_bitrev[2]].Im + p_src[p_bitrev[3]].Im;

    p_tmp[0].Re = tmp1 + tmp2;
    p_tmp[2].Re = tmp1 - tmp2;
    p_tmp[0].Im = tmp3 + tmp4;
    p_tmp[2].Im = tmp3 - tmp4;

    tmp1 = p_src[p_bitrev[0]].Re - p_src[p_bitrev[1]].Re;
    tmp2 = p_src[p_bitrev[2]].Re - p_src[p_bitrev[3]].Re;
    tmp3 = p_src[p_bitrev[0]].Im - p_src[p_bitrev[1]].Im;
    tmp4 = p_src[p_bitrev[2]].Im - p_src[p_bitrev[3]].Im;

    p_tmp[1].Re = tmp1 + tmp4;
    p_tmp[3].Re = tmp1 - tmp4;
    p_tmp[1].Im = tmp3 - tmp2;
    p_tmp[3].Im = tmp3 + tmp2;
  }

  /*
   * Loop performing sub-transforms of size 8,
   * which contain four butterfly operations.
   */
  num_transforms = (num_transforms >> 1) | 1;
  for (uint32_t n = 0; n < num_transforms; ++n) {
    OMX_U32 offset = pFFTSpec->pOffset[n] << 3;
    OMX_U16* p_bitrev = pFFTSpec->pBitRev + offset;
    OMX_FC32* p_tmp = p_buf + offset;
    const OMX_FC32* p_src = (const OMX_FC32*)pSrc;

    tmp1 = p_src[p_bitrev[4]].Re + p_src[p_bitrev[5]].Re;
    tmp2 = p_src[p_bitrev[6]].Re + p_src[p_bitrev[7]].Re;
    tmp3 = p_src[p_bitrev[4]].Im + p_src[p_bitrev[5]].Im;
    tmp4 = p_src[p_bitrev[6]].Im + p_src[p_bitrev[7]].Im;

    tmp5 = tmp1 + tmp2;
    tmp1 = tmp1 - tmp2;
    tmp2 = tmp3 + tmp4;
    tmp3 = tmp3 - tmp4;

    p_tmp[4].Re = p_tmp[0].Re - tmp5;
    p_tmp[0].Re = p_tmp[0].Re + tmp5;
    p_tmp[4].Im = p_tmp[0].Im - tmp2;
    p_tmp[0].Im = p_tmp[0].Im + tmp2;
    p_tmp[6].Re = p_tmp[2].Re - tmp3;
    p_tmp[2].Re = p_tmp[2].Re + tmp3;
    p_tmp[6].Im = p_tmp[2].Im + tmp1;
    p_tmp[2].Im = p_tmp[2].Im - tmp1;

    tmp1 = p_src[p_bitrev[4]].Re - p_src[p_bitrev[5]].Re;
    tmp2 = p_src[p_bitrev[6]].Re - p_src[p_bitrev[7]].Re;
    tmp3 = p_src[p_bitrev[4]].Im - p_src[p_bitrev[5]].Im;
    tmp4 = p_src[p_bitrev[6]].Im - p_src[p_bitrev[7]].Im;

    tmp5 = SQRT1_2 * (tmp1 + tmp3);
    tmp1 = SQRT1_2 * (tmp3 - tmp1);
    tmp3 = SQRT1_2 * (tmp2 - tmp4);
    tmp2 = SQRT1_2 * (tmp2 + tmp4);

    tmp4 = tmp5 + tmp3;
    tmp5 = tmp5 - tmp3;
    tmp3 = tmp1 + tmp2;
    tmp1 = tmp1 - tmp2;

    p_tmp[5].Re = p_tmp[1].Re - tmp4;
    p_tmp[1].Re = p_tmp[1].Re + tmp4;
    p_tmp[5].Im = p_tmp[1].Im - tmp3;
    p_tmp[1].Im = p_tmp[1].Im + tmp3;
    p_tmp[7].Re = p_tmp[3].Re - tmp1;
    p_tmp[3].Re = p_tmp[3].Re + tmp1;
    p_tmp[7].Im = p_tmp[3].Im + tmp5;
    p_tmp[3].Im = p_tmp[3].Im - tmp5;
  }

  step = 1 << (pFFTSpec->order - 4);
  n1_4 = 4; /* Quarter of the sub-transform size. */
  /* Outer loop that loops over FFT stages. */
  for (uint32_t fft_stage = 4; fft_stage <= pFFTSpec->order - 1; ++fft_stage) {
    OMX_U32 n1_2 = 2 * n1_4;
    OMX_U32 n3_4 = 3 * n1_4;
    num_transforms = (num_transforms >> 1) | 1;
    /*
     * Loop performing sub-transforms of size 16 and higher.
     * The actual size depends on the stage.
     */
    for (uint32_t n = 0; n < num_transforms; ++n) {
      OMX_U32 offset = pFFTSpec->pOffset[n] << fft_stage;
      OMX_FC32* p_tmp = p_buf + offset;

      tmp1 = p_tmp[n1_2].Re + p_tmp[n3_4].Re;
      tmp2 = p_tmp[n1_2].Re - p_tmp[n3_4].Re;
      tmp3 = p_tmp[n1_2].Im + p_tmp[n3_4].Im;
      tmp4 = p_tmp[n1_2].Im - p_tmp[n3_4].Im;

      p_tmp[n1_2].Re = p_tmp[0].Re - tmp1;
      p_tmp[n1_2].Im = p_tmp[0].Im - tmp3;
      p_tmp[0].Re = p_tmp[0].Re + tmp1;
      p_tmp[0].Im = p_tmp[0].Im + tmp3;
      p_tmp[n3_4].Re = p_tmp[n1_4].Re - tmp4;
      p_tmp[n3_4].Im = p_tmp[n1_4].Im + tmp2;
      p_tmp[n1_4].Re = p_tmp[n1_4].Re + tmp4;
      p_tmp[n1_4].Im = p_tmp[n1_4].Im - tmp2;

      /* Twiddle table is initialized for the maximal FFT size. */
      w_re_ptr = pFFTSpec->pTwiddle + step;
      w_im_ptr =
          pFFTSpec->pTwiddle + (OMX_U32)(1 << (pFFTSpec->order - 2)) - step;

      /*
       * Loop performing split-radix butterfly operations for
       * one sub-transform.
       */
      for (uint32_t i = 1; i < n1_4; ++i) {
        w_re = *w_re_ptr;
        w_im = *w_im_ptr;

        tmp1 = w_re * p_tmp[n1_2 + i].Re + w_im * p_tmp[n1_2 + i].Im;
        tmp2 = w_re * p_tmp[n1_2 + i].Im - w_im * p_tmp[n1_2 + i].Re;
        tmp3 = w_re * p_tmp[n3_4 + i].Re - w_im * p_tmp[n3_4 + i].Im;
        tmp4 = w_re * p_tmp[n3_4 + i].Im + w_im * p_tmp[n3_4 + i].Re;

        tmp5 = tmp1 + tmp3;
        tmp1 = tmp1 - tmp3;
        tmp6 = tmp2 + tmp4;
        tmp2 = tmp2 - tmp4;

        p_tmp[n1_2 + i].Re = p_tmp[i].Re - tmp5;
        p_tmp[n1_2 + i].Im = p_tmp[i].Im - tmp6;
        p_tmp[i].Re = p_tmp[i].Re + tmp5;
        p_tmp[i].Im = p_tmp[i].Im + tmp6;
        p_tmp[n3_4 + i].Re = p_tmp[n1_4 + i].Re - tmp2;
        p_tmp[n3_4 + i].Im = p_tmp[n1_4 + i].Im + tmp1;
        p_tmp[n1_4 + i].Re = p_tmp[n1_4 + i].Re + tmp2;
        p_tmp[n1_4 + i].Im = p_tmp[n1_4 + i].Im - tmp1;

        w_re_ptr += step;
        w_im_ptr -= step;
      }
    }
    step >>= 1;
    n1_4 <<= 1;
  }

  /* Additional computation to get the output for full FFT size. */
  w_re_ptr = pFFTSpec->pTwiddle + step;
  w_im_ptr = pFFTSpec->pTwiddle + (OMX_U32)(1 << (pFFTSpec->order - 2)) - step;

  for (uint32_t i = 1; i < fft_size / 8; ++i) {
    tmp1 = p_buf[i].Re;
    tmp2 = p_buf[i].Im;
    tmp3 = p_buf[fft_size / 2 - i].Re;
    tmp4 = p_buf[fft_size / 2 - i].Im;

    tmp5 = tmp1 + tmp3;
    tmp6 = tmp1 - tmp3;
    tmp7 = tmp2 + tmp4;
    tmp8 = tmp2 - tmp4;

    tmp1 = p_buf[i + fft_size / 4].Re;
    tmp2 = p_buf[i + fft_size / 4].Im;
    tmp3 = p_buf[fft_size / 4 - i].Re;
    tmp4 = p_buf[fft_size / 4 - i].Im;

    w_re = *w_re_ptr;
    w_im = *w_im_ptr;

    p_dst[i].Re = 0.5f * (tmp5 + w_re * tmp7 - w_im * tmp6);
    p_dst[i].Im = 0.5f * (tmp8 - w_re * tmp6 - w_im * tmp7);
    p_dst[fft_size / 2 - i].Re = 0.5f * (tmp5 - w_re * tmp7 + w_im * tmp6);
    p_dst[fft_size / 2 - i].Im = 0.5f * (-tmp8 - w_re * tmp6 - w_im * tmp7);

    tmp5 = tmp1 + tmp3;
    tmp6 = tmp1 - tmp3;
    tmp7 = tmp2 + tmp4;
    tmp8 = tmp2 - tmp4;

    p_dst[i + fft_size / 4].Re = 0.5f * (tmp5 - w_im * tmp7 - w_re * tmp6);
    p_dst[i + fft_size / 4].Im = 0.5f * (tmp8 + w_im * tmp6 - w_re * tmp7);
    p_dst[fft_size / 4 - i].Re = 0.5f * (tmp5 + w_im * tmp7 + w_re * tmp6);
    p_dst[fft_size / 4 - i].Im = 0.5f * (-tmp8 + w_im * tmp6 - w_re * tmp7);

    w_re_ptr += step;
    w_im_ptr -= step;
  }
  tmp1 = p_buf[fft_size / 8].Re;
  tmp2 = p_buf[fft_size / 8].Im;
  tmp3 = p_buf[3 * fft_size / 8].Re;
  tmp4 = p_buf[3 * fft_size / 8].Im;

  tmp5 = tmp1 + tmp3;
  tmp6 = tmp1 - tmp3;
  tmp7 = tmp2 + tmp4;
  tmp8 = tmp2 - tmp4;

  w_re = *w_re_ptr;
  w_im = *w_im_ptr;

  p_dst[fft_size / 8].Re = 0.5f * (tmp5 + w_re * tmp7 - w_im * tmp6);
  p_dst[fft_size / 8].Im = 0.5f * (tmp8 - w_re * tmp6 - w_im * tmp7);
  p_dst[3 * fft_size / 8].Re = 0.5f * (tmp5 - w_re * tmp7 + w_im * tmp6);
  p_dst[3 * fft_size / 8].Im = 0.5f * (-tmp8 - w_re * tmp6 - w_im * tmp7);

  p_dst[0].Re = p_buf[0].Re + p_buf[0].Im;
  p_dst[fft_size / 4].Re = p_buf[fft_size / 4].Re;
  p_dst[fft_size / 2].Re = p_buf[0].Re - p_buf[0].Im;
  p_dst[0].Im = 0.0f;
  p_dst[fft_size / 4].Im = -p_buf[fft_size / 4].Im;
  p_dst[fft_size / 2].Im = 0.0f;

  return OMX_Sts_NoErr;
}
