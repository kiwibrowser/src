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

OMXResult mips_FFTInv_CCSToR_F32_real(const OMX_F32* pSrc,
                                      OMX_F32* pDst,
                                      const MIPSFFTSpec_R_FC32* pFFTSpec) {
  OMX_U32 num_transforms;
  OMX_FC32* p_buf = (OMX_FC32*)pFFTSpec->pBuf;
  const OMX_FC32* p_src = (const OMX_FC32*)pSrc;
  OMX_U32 fft_size = 1 << pFFTSpec->order;
  OMX_F32 factor, tmp1, tmp2;
  const OMX_F32* w_re_ptr;
  const OMX_F32* w_im_ptr;

  /* Copy the input into the auxiliary buffer. */
  for (uint32_t n = 1; n < fft_size / 2; ++n) {
    p_buf[pFFTSpec->pBitRevInv[n]] = p_src[n];
    p_buf[pFFTSpec->pBitRevInv[fft_size - n]].Re = p_src[n].Re;
    p_buf[pFFTSpec->pBitRevInv[fft_size - n]].Im = -p_src[n].Im;
  }
  p_buf[0] = p_src[0];
  p_buf[pFFTSpec->pBitRevInv[fft_size / 2]] = p_src[fft_size / 2];

  factor = (OMX_F32)1.0f / fft_size;

  /* Special case for order == 2, only one sub-transform of size 4. */
  if (pFFTSpec->order == 2) {
    OMX_F32 tmp3, tmp4;

    tmp1 = p_buf[0].Re + p_buf[1].Re;
    tmp2 = p_buf[2].Re + p_buf[3].Re;
    tmp3 = p_buf[0].Re - p_buf[1].Re;
    tmp4 = p_buf[2].Im - p_buf[3].Im;

    pDst[0] = factor * (tmp1 + tmp2);
    pDst[2] = factor * (tmp1 - tmp2);
    pDst[1] = factor * (tmp3 - tmp4);
    pDst[3] = factor * (tmp3 + tmp4);

    return OMX_Sts_NoErr;
  }

  /*
   * Loop performing sub-transforms of size 4,
   * which contain two butterfly operations.
   */
  num_transforms = (SUBTRANSFORM_CONST >> (16 - pFFTSpec->order)) | 1;
  for (uint32_t n = 0; n < num_transforms; ++n) {
    OMX_U32 offset = pFFTSpec->pOffset[n] << 2;
    OMX_FC32* p_tmp = p_buf + offset;
    OMX_F32 tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;

    tmp1 = p_tmp[0].Re + p_tmp[1].Re;
    tmp5 = p_tmp[2].Re + p_tmp[3].Re;
    tmp2 = p_tmp[0].Im + p_tmp[1].Im;
    tmp6 = p_tmp[2].Im + p_tmp[3].Im;
    tmp3 = p_tmp[0].Re - p_tmp[1].Re;
    tmp8 = p_tmp[2].Im - p_tmp[3].Im;
    tmp4 = p_tmp[0].Im - p_tmp[1].Im;
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

  num_transforms >>= 1;
  if (!num_transforms) {
    /*
     * Special case for order == 3, only one sub-transform of size 8.
     * Omit the unnecessary calculations and place the output
     * into the destination buffer.
     */
    OMX_F32 tmp3, tmp4, tmp5, tmp6;

    tmp1 = p_buf[4].Re + p_buf[5].Re;
    tmp2 = p_buf[6].Re + p_buf[7].Re;
    tmp3 = p_buf[4].Im + p_buf[5].Im;
    tmp4 = p_buf[6].Im + p_buf[7].Im;

    tmp5 = tmp1 + tmp2;
    tmp6 = tmp3 - tmp4;

    tmp1 = p_buf[4].Re - p_buf[5].Re;
    tmp2 = p_buf[4].Im - p_buf[5].Im;
    tmp3 = p_buf[6].Re - p_buf[7].Re;
    tmp4 = p_buf[6].Im - p_buf[7].Im;

    pDst[4] = factor * (p_buf[0].Re - tmp5);
    pDst[0] = factor * (p_buf[0].Re + tmp5);
    pDst[6] = factor * (p_buf[2].Re + tmp6);
    pDst[2] = factor * (p_buf[2].Re - tmp6);

    tmp5 = SQRT1_2 * (tmp1 - tmp2);
    tmp6 = SQRT1_2 * (tmp3 + tmp4);
    tmp1 = SQRT1_2 * (tmp1 + tmp2);
    tmp2 = SQRT1_2 * (tmp4 - tmp3);

    tmp3 = tmp5 + tmp6;
    tmp4 = tmp1 - tmp2;

    pDst[5] = factor * (p_buf[1].Re - tmp3);
    pDst[1] = factor * (p_buf[1].Re + tmp3);
    pDst[7] = factor * (p_buf[3].Re + tmp4);
    pDst[3] = factor * (p_buf[3].Re - tmp4);

    return OMX_Sts_NoErr;
  }

  num_transforms |= 1;
  for (uint32_t n = 0; n < num_transforms; ++n) {
    /*
     * Loop performing sub-transforms of size 8,
     * which contain four butterfly operations.
     */
    OMX_U32 offset = pFFTSpec->pOffset[n] << 3;
    OMX_FC32* p_tmp = p_buf + offset;
    OMX_F32 tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;

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

  /*
   * Last FFT stage, doing sub-transform of size 16. Avoid unnecessary
   * calculations and place the output directly into the destination buffer.
   */
  tmp1 = p_buf[8].Re + p_buf[12].Re;
  tmp2 = p_buf[8].Im - p_buf[12].Im;

  pDst[8] = factor * (p_buf[0].Re - tmp1);
  pDst[0] = factor * (p_buf[0].Re + tmp1);
  pDst[12] = factor * (p_buf[4].Re + tmp2);
  pDst[4] = factor * (p_buf[4].Re - tmp2);

  w_re_ptr = pFFTSpec->pTwiddle + 1;
  w_im_ptr = pFFTSpec->pTwiddle + (OMX_U32)(1 << (pFFTSpec->order - 2)) - 1;

  /* Loop performing split-radix butterfly operations. */
  for (uint32_t n = 1; n < 4; ++n) {
    OMX_F32 tmp3, tmp4, tmp5;
    OMX_F32 w_re = *w_re_ptr;
    OMX_F32 w_im = *w_im_ptr;

    tmp1 = w_re * p_buf[8 + n].Re - w_im * p_buf[8 + n].Im;
    tmp2 = w_re * p_buf[8 + n].Im + w_im * p_buf[8 + n].Re;
    tmp3 = w_re * p_buf[12 + n].Re + w_im * p_buf[12 + n].Im;
    tmp4 = w_re * p_buf[12 + n].Im - w_im * p_buf[12 + n].Re;

    tmp5 = tmp1 + tmp3;
    tmp2 = tmp2 - tmp4;

    pDst[8 + n] = factor * (p_buf[n].Re - tmp5);
    pDst[n] = factor * (p_buf[n].Re + tmp5);
    pDst[12 + n] = factor * (p_buf[4 + n].Re + tmp2);
    pDst[4 + n] = factor * (p_buf[4 + n].Re - tmp2);

    ++w_re_ptr;
    --w_im_ptr;
  }
  return OMX_Sts_NoErr;
}
