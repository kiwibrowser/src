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

OMXResult mips_FFTFwd_RToCCS_F32_real(const OMX_F32* pSrc,
                                      OMX_F32* pDst,
                                      const MIPSFFTSpec_R_FC32* pFFTSpec) {
  OMX_U32 num_transforms;
  OMX_FC32* p_dst = (OMX_FC32*)pDst;
  OMX_FC32* p_buf = (OMX_FC32*)pFFTSpec->pBuf;
  OMX_F32 tmp1, tmp2, tmp3, tmp4;
  const OMX_F32* w_re_ptr;
  const OMX_F32* w_im_ptr;

  /* Transform for order = 2. */
  /* TODO: hard-code the offsets for p_src. */
  if (pFFTSpec->order == 2) {
    OMX_U16* p_bitrev = pFFTSpec->pBitRev;

    tmp1 = pSrc[p_bitrev[0]] + pSrc[p_bitrev[1]];
    tmp2 = pSrc[p_bitrev[2]] + pSrc[p_bitrev[3]];
    tmp3 = pSrc[p_bitrev[0]] - pSrc[p_bitrev[1]];
    tmp4 = pSrc[p_bitrev[2]] - pSrc[p_bitrev[3]];

    p_dst[0].Re = tmp1 + tmp2;
    p_dst[2].Re = tmp1 - tmp2;
    p_dst[0].Im = 0.0f;
    p_dst[2].Im = 0.0f;
    p_dst[1].Re = tmp3;
    p_dst[1].Im = -tmp4;

    return OMX_Sts_NoErr;
  }

  /*
   * Loop performing sub-transforms of size 4, which contain two butterfly
   * operations. Reading the input signal from split-radix bitreverse offsets.
   */
  num_transforms = (SUBTRANSFORM_CONST >> (16 - pFFTSpec->order)) | 1;
  for (uint32_t n = 0; n < num_transforms; ++n) {
    OMX_U32 offset = pFFTSpec->pOffset[n] << 2;
    OMX_FC32* p_tmp = p_buf + offset;
    OMX_U16* p_bitrev = pFFTSpec->pBitRev + offset;

    tmp1 = pSrc[p_bitrev[0]] + pSrc[p_bitrev[1]];
    tmp2 = pSrc[p_bitrev[2]] + pSrc[p_bitrev[3]];
    tmp3 = pSrc[p_bitrev[0]] - pSrc[p_bitrev[1]];
    tmp4 = pSrc[p_bitrev[2]] - pSrc[p_bitrev[3]];

    p_tmp[0].Re = tmp1 + tmp2;
    p_tmp[2].Re = tmp1 - tmp2;
    p_tmp[0].Im = 0.0f;
    p_tmp[2].Im = 0.0f;
    p_tmp[1].Re = tmp3;
    p_tmp[3].Re = tmp3;
    p_tmp[1].Im = -tmp4;
    p_tmp[3].Im = tmp4;
  }

  /*
   * Loop performing sub-transforms of size 8,
   * which contain four butterfly operations.
   */
  num_transforms >>= 1;
  if (!num_transforms) {
    /*
     * Means the FFT size is equal to 8, so this is the last stage. Place the
     * output to the destination buffer and avoid unnecessary computations.
     */
    OMX_FC32* p_tmp = p_buf;
    OMX_U16* p_bitrev = pFFTSpec->pBitRev;
    OMX_F32 tmp5;

    tmp1 = pSrc[p_bitrev[4]] + pSrc[p_bitrev[5]];
    tmp2 = pSrc[p_bitrev[6]] + pSrc[p_bitrev[7]];
    tmp3 = tmp1 + tmp2;
    tmp4 = tmp1 - tmp2;

    tmp1 = pSrc[p_bitrev[4]] - pSrc[p_bitrev[5]];
    tmp2 = pSrc[p_bitrev[6]] - pSrc[p_bitrev[7]];
    tmp5 = SQRT1_2 * (tmp1 + tmp2);
    tmp1 = SQRT1_2 * (tmp1 - tmp2);

    p_dst[4].Re = p_tmp[0].Re - tmp3;
    p_dst[0].Re = p_tmp[0].Re + tmp3;
    p_dst[0].Im = p_tmp[0].Im;
    p_dst[4].Im = p_tmp[0].Im;
    p_dst[2].Re = p_tmp[2].Re;
    p_dst[2].Im = p_tmp[2].Im - tmp4;
    p_dst[1].Re = p_tmp[1].Re + tmp5;
    p_dst[1].Im = p_tmp[1].Im - tmp1;
    p_dst[3].Re = p_tmp[3].Re - tmp5;
    p_dst[3].Im = p_tmp[3].Im - tmp1;

    return OMX_Sts_NoErr;
  }

  num_transforms |= 1;

  for (uint32_t n = 0; n < num_transforms; ++n) {
    OMX_U32 offset = pFFTSpec->pOffset[n] << 3;
    OMX_FC32* p_tmp = p_buf + offset;
    OMX_U16* p_bitrev = pFFTSpec->pBitRev + offset;
    OMX_F32 tmp5;

    tmp1 = pSrc[p_bitrev[4]] + pSrc[p_bitrev[5]];
    tmp2 = pSrc[p_bitrev[6]] + pSrc[p_bitrev[7]];
    tmp3 = tmp1 + tmp2;
    tmp4 = tmp1 - tmp2;

    tmp1 = pSrc[p_bitrev[4]] - pSrc[p_bitrev[5]];
    tmp2 = pSrc[p_bitrev[6]] - pSrc[p_bitrev[7]];
    tmp5 = SQRT1_2 * (tmp1 + tmp2);
    tmp1 = SQRT1_2 * (tmp1 - tmp2);

    p_tmp[4].Re = p_tmp[0].Re - tmp3;
    p_tmp[0].Re = p_tmp[0].Re + tmp3;
    p_tmp[4].Im = p_tmp[0].Im;
    p_tmp[6].Re = p_tmp[2].Re;
    p_tmp[6].Im = p_tmp[2].Im + tmp4;
    p_tmp[2].Im = p_tmp[2].Im - tmp4;

    p_tmp[5].Re = p_tmp[1].Re - tmp5;
    p_tmp[1].Re = p_tmp[1].Re + tmp5;
    p_tmp[5].Im = p_tmp[1].Im + tmp1;
    p_tmp[1].Im = p_tmp[1].Im - tmp1;
    p_tmp[7].Re = p_tmp[3].Re + tmp5;
    p_tmp[3].Re = p_tmp[3].Re - tmp5;
    p_tmp[7].Im = p_tmp[3].Im + tmp1;
    p_tmp[3].Im = p_tmp[3].Im - tmp1;
  }

  /*
   * Last FFT stage,  performing sub-transforms of size 16. Place the output
   * into the destination buffer and avoid unnecessary computations.
   */
  tmp1 = p_buf[8].Re + p_buf[12].Re;
  tmp2 = p_buf[8].Re - p_buf[12].Re;
  tmp3 = p_buf[8].Im + p_buf[12].Im;
  tmp4 = p_buf[8].Im - p_buf[12].Im;

  p_dst[8].Re = p_buf[0].Re - tmp1;
  p_dst[0].Re = p_buf[0].Re + tmp1;
  p_dst[8].Im = p_buf[0].Im - tmp3;
  p_dst[0].Im = p_buf[0].Im + tmp3;
  p_dst[4].Re = p_buf[4].Re + tmp4;
  p_dst[4].Im = p_buf[4].Im - tmp2;

  w_re_ptr = pFFTSpec->pTwiddle + 1;
  w_im_ptr = pFFTSpec->pTwiddle + (OMX_U32)(1 << (pFFTSpec->order - 2)) - 1;

  /* Loop performing split-radix butterfly operations. */
  for (uint32_t n = 1; n < 4; ++n) {
    OMX_F32 tmp5, tmp6;
    OMX_F32 w_re = *w_re_ptr;
    OMX_F32 w_im = *w_im_ptr;

    tmp1 = w_re * p_buf[8 + n].Re + w_im * p_buf[8 + n].Im;
    tmp2 = w_re * p_buf[8 + n].Im - w_im * p_buf[8 + n].Re;
    tmp3 = w_re * p_buf[12 + n].Re - w_im * p_buf[12 + n].Im;
    tmp4 = w_re * p_buf[12 + n].Im + w_im * p_buf[12 + n].Re;

    tmp5 = tmp1 + tmp3;
    tmp1 = tmp1 - tmp3;
    tmp6 = tmp2 + tmp4;
    tmp2 = tmp2 - tmp4;

    p_dst[n].Re = p_buf[n].Re + tmp5;
    p_dst[n].Im = p_buf[n].Im + tmp6;
    p_dst[4 + n].Re = p_buf[4 + n].Re + tmp2;
    p_dst[4 + n].Im = p_buf[4 + n].Im - tmp1;

    ++w_re_ptr;
    --w_im_ptr;
  }
  return OMX_Sts_NoErr;
}
