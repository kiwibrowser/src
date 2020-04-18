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
#include "dl/sp/src/x86/x86SP_SSE_Math.h"

void x86SP_FFT_CToC_FC32_Inv_Radix4_ls_sse(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n) {
  OMX_INT n_by_2 = n >> 1;
  OMX_INT n_by_4 = n >> 2;
  OMX_INT n_mul_2 = n << 1;
  OMX_INT i;

  OMX_F32 *out0 = out;

  for (i = 0; i < n_by_2; i += 8) {
    const OMX_F32 *tw1 = twiddle + i;
    const OMX_F32 *tw2 = tw1 + i;
    const OMX_F32 *tw3 = tw2 + i;
    const OMX_F32 *in0 = in + (i << 1);
    const OMX_F32 *in1 = in0 + 4;
    const OMX_F32 *in2 = in1 + 4;
    const OMX_F32 *in3 = in2 + 4;
    OMX_F32 *out1 = out0 + n_by_4;
    OMX_F32 *out2 = out1 + n_by_4;
    OMX_F32 *out3 = out2 + n_by_4;

    VC v_tw1;
    VC v_tw2;
    VC v_tw3;
    VC v_t0;
    VC v_t1;
    VC v_t2;
    VC v_t3;
    VC v_t4;
    VC v_t5;
    VC v_t6;
    VC v_t7;

    v_tw1.real = _mm_set_ps(tw1[6], tw1[4], tw1[2], tw1[0]);
    v_tw1.imag = _mm_set_ps(
        tw1[6 + n_mul_2],
        tw1[4 + n_mul_2],
        tw1[2 + n_mul_2],
        tw1[n_mul_2]);
    v_tw2.real = _mm_set_ps(tw2[12], tw2[8], tw2[4], tw2[0]);
    v_tw2.imag = _mm_set_ps(
        tw2[12 + n_mul_2],
        tw2[8 + n_mul_2],
        tw2[4 + n_mul_2],
        tw2[n_mul_2]);
    v_tw3.real = _mm_set_ps(tw3[18], tw3[12], tw3[6], tw3[0]);
    v_tw3.imag = _mm_set_ps(
        tw3[18 + n_mul_2],
        tw3[12 + n_mul_2],
        tw3[6 + n_mul_2],
        tw3[n_mul_2]);

    VC_LOAD_MATRIX_TRANSPOSE(&v_t0, &v_t1, &v_t2, &v_t3, in0, in1, in2, in3, n);

    RADIX4_INV_BUTTERFLY(&v_t4, &v_t5, &v_t6, &v_t7,
                         &v_tw1, &v_tw2, &v_tw3,
                         &v_t0, &v_t1, &v_t2, &v_t3);

    RADIX4_INV_BUTTERFLY_STORE(out0, out1, out2, out3,
                               &v_t4, &v_t5, &v_t6, &v_t7, n);

    out0 += 4;
  }
}
