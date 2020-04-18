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

// This function handles the case when set_count = 2, in which we cannot
// unroll the set loop by 4 to meet the SSE requirement (4 elements).
static void InternalUnroll2Inv(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n) {
  OMX_INT i;
  OMX_INT n_by_2 = n >> 1;
  OMX_INT n_by_4 = n >> 2;
  OMX_INT n_mul_2 = n << 1;
  OMX_F32 *out0 = out;

  for (i = 0; i < n_by_2; i += 8) {
    const OMX_F32 *tw1  = twiddle + i;
    const OMX_F32 *tw2  = tw1 + i;
    const OMX_F32 *tw3  = tw2 + i;
    const OMX_F32 *tw1e = tw1 + 4;
    const OMX_F32 *tw2e = tw2 + 8;
    const OMX_F32 *tw3e = tw3 + 12;

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

    v_tw1.real = _mm_shuffle_ps(_mm_load_ss(tw1),
                                _mm_load_ss(tw1e),
                                _MM_SHUFFLE(0, 0, 0, 0));
    v_tw1.imag = _mm_shuffle_ps(_mm_load_ss(tw1 + n_mul_2),
                                _mm_load_ss(tw1e + n_mul_2),
                                _MM_SHUFFLE(0, 0, 0, 0));
    v_tw2.real = _mm_shuffle_ps(_mm_load_ss(tw2),
                                _mm_load_ss(tw2e),
                                _MM_SHUFFLE(0, 0, 0, 0));
    v_tw2.imag = _mm_shuffle_ps(_mm_load_ss(tw2 + n_mul_2),
                                _mm_load_ss(tw2e + n_mul_2),
                                _MM_SHUFFLE(0, 0, 0, 0));
    v_tw3.real = _mm_shuffle_ps(_mm_load_ss(tw3),
                                _mm_load_ss(tw3e),
                                _MM_SHUFFLE(0, 0, 0, 0));
    v_tw3.imag = _mm_shuffle_ps(_mm_load_ss(tw3 + n_mul_2),
                                _mm_load_ss(tw3e + n_mul_2),
                                _MM_SHUFFLE(0, 0, 0, 0));

    __m128 xmm0;
    __m128 xmm1;
    __m128 xmm2;
    __m128 xmm3;
    __m128 xmm4;
    __m128 xmm5;
    __m128 xmm6;
    __m128 xmm7;

    const OMX_F32 *in0 = in + (i << 1);
    xmm0 = _mm_load_ps(in0);
    xmm1 = _mm_load_ps(in0 + 4);
    xmm2 = _mm_load_ps(in0 + 8);
    xmm3 = _mm_load_ps(in0 + 12);
    v_t0.real = _mm_shuffle_ps(xmm0, xmm2, _MM_SHUFFLE(1, 0, 1, 0));
    v_t1.real = _mm_shuffle_ps(xmm0, xmm2, _MM_SHUFFLE(3, 2, 3, 2));
    v_t2.real = _mm_shuffle_ps(xmm1, xmm3, _MM_SHUFFLE(1, 0, 1, 0));
    v_t3.real = _mm_shuffle_ps(xmm1, xmm3, _MM_SHUFFLE(3, 2, 3, 2));

    xmm4 = _mm_load_ps(in0 + n);
    xmm5 = _mm_load_ps(in0 + n + 4);
    xmm6 = _mm_load_ps(in0 + n + 8);
    xmm7 = _mm_load_ps(in0 + n + 12);
    v_t0.imag = _mm_shuffle_ps(xmm4, xmm6, _MM_SHUFFLE(1, 0, 1, 0));
    v_t1.imag = _mm_shuffle_ps(xmm4, xmm6, _MM_SHUFFLE(3, 2, 3, 2));
    v_t2.imag = _mm_shuffle_ps(xmm5, xmm7, _MM_SHUFFLE(1, 0, 1, 0));
    v_t3.imag = _mm_shuffle_ps(xmm5, xmm7, _MM_SHUFFLE(3, 2, 3, 2));

    OMX_F32 *out1 = out0 + n_by_4;
    OMX_F32 *out2 = out1 + n_by_4;
    OMX_F32 *out3 = out2 + n_by_4;

    RADIX4_INV_BUTTERFLY(&v_t4, &v_t5, &v_t6, &v_t7,
                         &v_tw1, &v_tw2, &v_tw3,
                         &v_t0, &v_t1, &v_t2, &v_t3);

    RADIX4_INV_BUTTERFLY_STORE(out0, out1, out2, out3,
                               &v_t4, &v_t5, &v_t6, &v_t7, n);

    out0 += 4;
  }
}

void x86SP_FFT_CToC_FC32_Inv_Radix4_ms_sse(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n,
    OMX_INT sub_size,
    OMX_INT sub_num) {
  OMX_INT set;
  OMX_INT grp;
  OMX_INT step = sub_num >> 1;
  OMX_INT set_count = sub_num >> 2;
  OMX_INT n_by_4 = n >> 2;
  OMX_INT n_mul_2 = n << 1;

  OMX_F32 *out0 = out;

  if (set_count == 2) {
    InternalUnroll2Inv(in, out, twiddle, n);
    return;
  }

  // grp == 0
  for (set = 0; set < set_count; set += 4) {
    const OMX_F32 * in0 = in + set;
    const OMX_F32 *in1 = in0 + set_count;
    const OMX_F32 *in2 = in1 + set_count;
    const OMX_F32 *in3 = in2 + set_count;

    VC v_t0;
    VC v_t1;
    VC v_t2;
    VC v_t3;
    VC v_t4;
    VC v_t5;
    VC v_t6;
    VC v_t7;

    VC_LOAD_SPLIT(&v_t0, in0, n);
    VC_LOAD_SPLIT(&v_t1, in1, n);
    VC_LOAD_SPLIT(&v_t2, in2, n);
    VC_LOAD_SPLIT(&v_t3, in3, n);

    OMX_F32 *out1 = out0 + n_by_4;
    OMX_F32 *out2 = out1 + n_by_4;
    OMX_F32 *out3 = out2 + n_by_4;

    RADIX4_BUTTERFLY_FS(&v_t4, &v_t5, &v_t6, &v_t7,
                        &v_t0, &v_t1, &v_t2, &v_t3);

    RADIX4_INV_BUTTERFLY_STORE(out0, out1, out2, out3,
                               &v_t4, &v_t5, &v_t6, &v_t7, n);

    out0 += 4;
  }

  for (grp = 1; grp < sub_size; ++grp) {
    const OMX_F32 *tw1 = twiddle + grp * step;
    const OMX_F32 *tw2 = tw1 + grp * step;
    const OMX_F32 *tw3 = tw2 + grp * step;

    VC v_tw1;
    VC v_tw2;
    VC v_tw3;

    v_tw1.real = _mm_load1_ps(tw1);
    v_tw1.imag = _mm_load1_ps(tw1 + n_mul_2);
    v_tw2.real = _mm_load1_ps(tw2);
    v_tw2.imag = _mm_load1_ps(tw2 + n_mul_2);
    v_tw3.real = _mm_load1_ps(tw3);
    v_tw3.imag = _mm_load1_ps(tw3 + n_mul_2);

    for (set = 0; set < set_count; set += 4) {
      const OMX_F32 *in0 = in + set + grp * sub_num;
      const OMX_F32 *in1 = in0 + set_count;
      const OMX_F32 *in2 = in1 + set_count;
      const OMX_F32 *in3 = in2 + set_count;

      VC v_t0;
      VC v_t1;
      VC v_t2;
      VC v_t3;
      VC v_t4;
      VC v_t5;
      VC v_t6;
      VC v_t7;

      VC_LOAD_SPLIT(&v_t0, in0, n);
      VC_LOAD_SPLIT(&v_t1, in1, n);
      VC_LOAD_SPLIT(&v_t2, in2, n);
      VC_LOAD_SPLIT(&v_t3, in3, n);

      OMX_F32 *out1 = out0 + n_by_4;
      OMX_F32 *out2 = out1 + n_by_4;
      OMX_F32 *out3 = out2 + n_by_4;

      RADIX4_INV_BUTTERFLY(&v_t4, &v_t5, &v_t6, &v_t7,
                           &v_tw1, &v_tw2, &v_tw3,
                           &v_t0, &v_t1, &v_t2, &v_t3);

      RADIX4_INV_BUTTERFLY_STORE(out0, out1, out2, out3,
                                 &v_t4, &v_t5, &v_t6, &v_t7, n);

      out0 += 4;
    }
  }
}
