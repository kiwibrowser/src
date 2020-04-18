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

OMXResult mips_FFTInv_CCSToR_F32_complex(const OMX_F32* pSrc,
                                         OMX_F32* pDst,
                                         const MIPSFFTSpec_R_FC32* pFFTSpec) {
  OMX_U32 num_transforms, step;
  /* Quarter, half and three-quarters of transform size. */
  OMX_U32 n1_4, n1_2, n3_4;
  const OMX_F32* w_re_ptr;
  const OMX_F32* w_im_ptr;
  OMX_U32 fft_size = 1 << pFFTSpec->order;
  OMX_FC32* p_buf = (OMX_FC32*)pFFTSpec->pBuf;
  const OMX_FC32* p_src = (const OMX_FC32*)pSrc;
  OMX_FC32* p_dst;
  OMX_U16* p_bitrev = pFFTSpec->pBitRevInv;
  OMX_F32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, factor;
  OMX_F32 w_re, w_im;

  w_re_ptr = pFFTSpec->pTwiddle + 1;
  w_im_ptr = pFFTSpec->pTwiddle + (OMX_U32)(1 << (pFFTSpec->order - 2)) - 1;

  /*
   * Preliminary loop performing input adaptation due to computing real FFT
   * through complex FFT of half the size.
   */
  for (uint32_t n = 1; n < fft_size / 8; ++n) {
    tmp1 = p_src[n].Re;
    tmp2 = p_src[n].Im;
    tmp3 = p_src[fft_size / 2 - n].Re;
    tmp4 = p_src[fft_size / 2 - n].Im;

    tmp5 = tmp1 + tmp3;
    tmp6 = tmp1 - tmp3;
    tmp7 = tmp2 + tmp4;
    tmp8 = tmp2 - tmp4;

    w_re = *w_re_ptr;
    w_im = *w_im_ptr;

    p_buf[p_bitrev[n]].Re = 0.5f * (tmp5 - w_re * tmp7 - w_im * tmp6);
    p_buf[p_bitrev[n]].Im = 0.5f * (tmp8 + w_re * tmp6 - w_im * tmp7);
    p_buf[p_bitrev[fft_size / 2 - n]].Re =
        0.5f * (tmp5 + w_re * tmp7 + w_im * tmp6);
    p_buf[p_bitrev[fft_size / 2 - n]].Im =
        0.5f * (-tmp8 + w_re * tmp6 - w_im * tmp7);

    tmp1 = p_src[n + fft_size / 4].Re;
    tmp2 = p_src[n + fft_size / 4].Im;
    tmp3 = p_src[fft_size / 4 - n].Re;
    tmp4 = p_src[fft_size / 4 - n].Im;

    tmp5 = tmp1 + tmp3;
    tmp6 = tmp1 - tmp3;
    tmp7 = tmp2 + tmp4;
    tmp8 = tmp2 - tmp4;

    p_buf[p_bitrev[n + fft_size / 4]].Re =
        0.5f * (tmp5 + w_im * tmp7 - w_re * tmp6);
    p_buf[p_bitrev[n + fft_size / 4]].Im =
        0.5f * (tmp8 - w_im * tmp6 - w_re * tmp7);
    p_buf[p_bitrev[fft_size / 4 - n]].Re =
        0.5f * (tmp5 - w_im * tmp7 + w_re * tmp6);
    p_buf[p_bitrev[fft_size / 4 - n]].Im =
        0.5f * (-tmp8 - w_im * tmp6 - w_re * tmp7);

    ++w_re_ptr;
    --w_im_ptr;
  }
  tmp1 = p_src[fft_size / 8].Re;
  tmp2 = p_src[fft_size / 8].Im;
  tmp3 = p_src[3 * fft_size / 8].Re;
  tmp4 = p_src[3 * fft_size / 8].Im;

  tmp5 = tmp1 + tmp3;
  tmp6 = tmp1 - tmp3;
  tmp7 = tmp2 + tmp4;
  tmp8 = tmp2 - tmp4;

  w_re = *w_re_ptr;
  w_im = *w_im_ptr;

  p_buf[p_bitrev[fft_size / 8]].Re = 0.5f * (tmp5 - w_re * tmp7 - w_im * tmp6);
  p_buf[p_bitrev[fft_size / 8]].Im = 0.5f * (tmp8 + w_re * tmp6 - w_im * tmp7);
  p_buf[p_bitrev[3 * fft_size / 8]].Re =
      0.5f * (tmp5 + w_re * tmp7 + w_im * tmp6);
  p_buf[p_bitrev[3 * fft_size / 8]].Im =
      0.5f * (-tmp8 + w_re * tmp6 - w_im * tmp7);

  p_buf[p_bitrev[0]].Re = 0.5f * (p_src[0].Re + p_src[fft_size / 2].Re);
  p_buf[p_bitrev[0]].Im = 0.5f * (p_src[0].Re - p_src[fft_size / 2].Re);
  p_buf[p_bitrev[fft_size / 4]].Re = p_src[fft_size / 4].Re;
  p_buf[p_bitrev[fft_size / 4]].Im = -p_src[fft_size / 4].Im;

  /*
   * Loop performing sub-transforms of size 4,
   * which contain 2 butterfly operations.
   */
  num_transforms = (SUBTRANSFORM_CONST >> (17 - pFFTSpec->order)) | 1;
  for (uint32_t n = 0; n < num_transforms; ++n) {
    OMX_U32 offset = pFFTSpec->pOffset[n] << 2;
    OMX_FC32* p_tmp = p_buf + offset;

    tmp1 = p_tmp[0].Re + p_tmp[1].Re;
    tmp2 = p_tmp[0].Im + p_tmp[1].Im;
    tmp3 = p_tmp[0].Re - p_tmp[1].Re;
    tmp4 = p_tmp[0].Im - p_tmp[1].Im;
    tmp5 = p_tmp[2].Re + p_tmp[3].Re;
    tmp6 = p_tmp[2].Im + p_tmp[3].Im;
    tmp8 = p_tmp[2].Im - p_tmp[3].Im;
    tmp7 = p_tmp[2].Re - p_tmp[3].Re;

    p_tmp[0].Re = tmp1 + tmp5;
    p_tmp[2].Re = tmp1 - tmp5;
    p_tmp[0].Im = tmp2 + tmp6;
    p_tmp[2].Im = tmp2 - tmp6;
    p_tmp[1].Re = tmp3 - tmp8;
    p_tmp[3].Re = tmp3 + tmp8;
    p_tmp[1].Im = tmp4 + tmp7;
    p_tmp[3].Im = tmp4 - tmp7;
  }

  num_transforms = (num_transforms >> 1) | 1;
  /*
   * Loop performing sub-transforms of size 8,
   * which contain four butterfly operations.
   */
  for (uint32_t n = 0; n < num_transforms; ++n) {
    OMX_U32 offset = pFFTSpec->pOffset[n] << 3;
    OMX_FC32* p_tmp = p_buf + offset;

    tmp1 = p_tmp[4].Re + p_tmp[5].Re;
    tmp3 = p_tmp[6].Re + p_tmp[7].Re;
    tmp2 = p_tmp[4].Im + p_tmp[5].Im;
    tmp4 = p_tmp[6].Im + p_tmp[7].Im;

    tmp5 = tmp1 + tmp3;
    tmp7 = tmp1 - tmp3;
    tmp6 = tmp2 + tmp4;
    tmp8 = tmp2 - tmp4;

    tmp1 = p_tmp[4].Re - p_tmp[5].Re;
    tmp2 = p_tmp[4].Im - p_tmp[5].Im;
    tmp3 = p_tmp[6].Re - p_tmp[7].Re;
    tmp4 = p_tmp[6].Im - p_tmp[7].Im;

    p_tmp[4].Re = p_tmp[0].Re - tmp5;
    p_tmp[0].Re = p_tmp[0].Re + tmp5;
    p_tmp[4].Im = p_tmp[0].Im - tmp6;
    p_tmp[0].Im = p_tmp[0].Im + tmp6;
    p_tmp[6].Re = p_tmp[2].Re + tmp8;
    p_tmp[2].Re = p_tmp[2].Re - tmp8;
    p_tmp[6].Im = p_tmp[2].Im - tmp7;
    p_tmp[2].Im = p_tmp[2].Im + tmp7;

    tmp5 = SQRT1_2 * (tmp1 - tmp2);
    tmp7 = SQRT1_2 * (tmp3 + tmp4);
    tmp6 = SQRT1_2 * (tmp1 + tmp2);
    tmp8 = SQRT1_2 * (tmp4 - tmp3);

    tmp1 = tmp5 + tmp7;
    tmp3 = tmp5 - tmp7;
    tmp2 = tmp6 + tmp8;
    tmp4 = tmp6 - tmp8;

    p_tmp[5].Re = p_tmp[1].Re - tmp1;
    p_tmp[1].Re = p_tmp[1].Re + tmp1;
    p_tmp[5].Im = p_tmp[1].Im - tmp2;
    p_tmp[1].Im = p_tmp[1].Im + tmp2;
    p_tmp[7].Re = p_tmp[3].Re + tmp4;
    p_tmp[3].Re = p_tmp[3].Re - tmp4;
    p_tmp[7].Im = p_tmp[3].Im - tmp3;
    p_tmp[3].Im = p_tmp[3].Im + tmp3;
  }

  step = 1 << (pFFTSpec->order - 4);
  n1_4 = 4;
  /* Outer loop that loops over FFT stages. */
  for (uint32_t fft_stage = 4; fft_stage <= pFFTSpec->order - 2; ++fft_stage) {
    n1_2 = 2 * n1_4;
    n3_4 = 3 * n1_4;
    num_transforms = (num_transforms >> 1) | 1;
    for (uint32_t n = 0; n < num_transforms; ++n) {
      OMX_U32 offset = pFFTSpec->pOffset[n] << fft_stage;
      OMX_FC32* p_tmp = p_buf + offset;

      tmp1 = p_tmp[n1_2].Re + p_tmp[n3_4].Re;
      tmp2 = p_tmp[n1_2].Re - p_tmp[n3_4].Re;
      tmp3 = p_tmp[n1_2].Im + p_tmp[n3_4].Im;
      tmp4 = p_tmp[n1_2].Im - p_tmp[n3_4].Im;

      p_tmp[n1_2].Re = p_tmp[0].Re - tmp1;
      p_tmp[0].Re = p_tmp[0].Re + tmp1;
      p_tmp[n1_2].Im = p_tmp[0].Im - tmp3;
      p_tmp[0].Im = p_tmp[0].Im + tmp3;
      p_tmp[n3_4].Re = p_tmp[n1_4].Re + tmp4;
      p_tmp[n1_4].Re = p_tmp[n1_4].Re - tmp4;
      p_tmp[n3_4].Im = p_tmp[n1_4].Im - tmp2;
      p_tmp[n1_4].Im = p_tmp[n1_4].Im + tmp2;

      w_re_ptr = pFFTSpec->pTwiddle + step;
      w_im_ptr =
          pFFTSpec->pTwiddle + (OMX_U32)(1 << (pFFTSpec->order - 2)) - step;

      /*
       * Loop performing split-radix butterfly operations for one sub-transform.
       */
      for (uint32_t i = 1; i < n1_4; ++i) {
        w_re = *w_re_ptr;
        w_im = *w_im_ptr;

        tmp1 = w_re * p_tmp[n1_2 + i].Re - w_im * p_tmp[n1_2 + i].Im;
        tmp2 = w_re * p_tmp[n1_2 + i].Im + w_im * p_tmp[n1_2 + i].Re;
        tmp3 = w_re * p_tmp[n3_4 + i].Re + w_im * p_tmp[n3_4 + i].Im;
        tmp4 = w_re * p_tmp[n3_4 + i].Im - w_im * p_tmp[n3_4 + i].Re;

        tmp5 = tmp1 + tmp3;
        tmp1 = tmp1 - tmp3;
        tmp6 = tmp2 + tmp4;
        tmp2 = tmp2 - tmp4;

        p_tmp[n1_2 + i].Re = p_tmp[i].Re - tmp5;
        p_tmp[i].Re = p_tmp[i].Re + tmp5;
        p_tmp[n1_2 + i].Im = p_tmp[i].Im - tmp6;
        p_tmp[i].Im = p_tmp[i].Im + tmp6;
        p_tmp[n3_4 + i].Re = p_tmp[n1_4 + i].Re + tmp2;
        p_tmp[n1_4 + i].Re = p_tmp[n1_4 + i].Re - tmp2;
        p_tmp[n3_4 + i].Im = p_tmp[n1_4 + i].Im - tmp1;
        p_tmp[n1_4 + i].Im = p_tmp[n1_4 + i].Im + tmp1;

        w_re_ptr += step;
        w_im_ptr -= step;
      }
    }
    step >>= 1;
    n1_4 <<= 1;
  }

  /* Last FFT stage, write data to output buffer. */
  n1_2 = 2 * n1_4;
  n3_4 = 3 * n1_4;
  factor = (OMX_F32)2.0f / fft_size;

  p_dst = (OMX_FC32*)pDst;

  tmp1 = p_buf[n1_2].Re + p_buf[n3_4].Re;
  tmp2 = p_buf[n1_2].Re - p_buf[n3_4].Re;
  tmp3 = p_buf[n1_2].Im + p_buf[n3_4].Im;
  tmp4 = p_buf[n1_2].Im - p_buf[n3_4].Im;

  p_dst[n1_2].Re = factor * (p_buf[0].Re - tmp1);
  p_dst[0].Re = factor * (p_buf[0].Re + tmp1);
  p_dst[n1_2].Im = factor * (p_buf[0].Im - tmp3);
  p_dst[0].Im = factor * (p_buf[0].Im + tmp3);
  p_dst[n3_4].Re = factor * (p_buf[n1_4].Re + tmp4);
  p_dst[n1_4].Re = factor * (p_buf[n1_4].Re - tmp4);
  p_dst[n3_4].Im = factor * (p_buf[n1_4].Im - tmp2);
  p_dst[n1_4].Im = factor * (p_buf[n1_4].Im + tmp2);

  w_re_ptr = pFFTSpec->pTwiddle + step;
  w_im_ptr = pFFTSpec->pTwiddle + (OMX_U32)(1 << (pFFTSpec->order - 2)) - step;

  for (uint32_t i = 1; i < n1_4; ++i) {
    w_re = *w_re_ptr;
    w_im = *w_im_ptr;

    tmp1 = w_re * p_buf[n1_2 + i].Re - w_im * p_buf[n1_2 + i].Im;
    tmp2 = w_re * p_buf[n1_2 + i].Im + w_im * p_buf[n1_2 + i].Re;
    tmp3 = w_re * p_buf[n3_4 + i].Re + w_im * p_buf[n3_4 + i].Im;
    tmp4 = w_re * p_buf[n3_4 + i].Im - w_im * p_buf[n3_4 + i].Re;

    tmp5 = tmp1 + tmp3;
    tmp1 = tmp1 - tmp3;
    tmp6 = tmp2 + tmp4;
    tmp2 = tmp2 - tmp4;

    p_dst[n1_2 + i].Re = factor * (p_buf[i].Re - tmp5);
    p_dst[i].Re = factor * (p_buf[i].Re + tmp5);
    p_dst[n1_2 + i].Im = factor * (p_buf[i].Im - tmp6);
    p_dst[i].Im = factor * (p_buf[i].Im + tmp6);
    p_dst[n3_4 + i].Re = factor * (p_buf[n1_4 + i].Re + tmp2);
    p_dst[n1_4 + i].Re = factor * (p_buf[n1_4 + i].Re - tmp2);
    p_dst[n3_4 + i].Im = factor * (p_buf[n1_4 + i].Im - tmp1);
    p_dst[n1_4 + i].Im = factor * (p_buf[n1_4 + i].Im + tmp1);

    w_re_ptr += step;
    w_im_ptr -= step;
  }

  return OMX_Sts_NoErr;
}
