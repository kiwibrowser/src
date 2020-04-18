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

#include <stdbool.h>
#include <stdint.h>

#include "dl/api/omxtypes.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/api/x86SP.h"
#include "dl/sp/src/x86/x86SP_SSE_Math.h"

extern OMX_F32* x86SP_F32_radix2_kernel_OutOfPlace(
    const OMX_F32 *src,
    OMX_F32 *buf1,
    OMX_F32 *buf2,
    const OMX_F32 *twiddle,
    OMX_INT n,
    bool forward_fft);

extern OMX_F32* x86SP_F32_radix4_kernel_OutOfPlace_sse(
    const OMX_F32 *src,
    OMX_F32 *buf1,
    OMX_F32 *buf2,
    const OMX_F32 *twiddle,
    OMX_INT n,
    bool forward_fft);

/**
 * A two-for-one algorithm is used here to do the real ifft:
 *
 * Input X[k], (k = 0, ..., N - 1)
 * Output x[n] = IDFT(N, k){X}
 * X' is complex conjugate of X
 * A[k] = (X[k] + X'[N/2 - k]) / 2
 * B[k] = (X[k] - X'[N/2 - k]) / 2 * W[k], (W = exp(j*2*PI*k/N);
 *                                          k = 0, ..., N/2 - 1)
 * Z[k] = A[k] + j * B[k], (k = 0, ..., N/2 - 1)
 * z[n] = IDFT(N/2, k){Z}
 * x[2n] = Re(z[n]), (n = 0, ..., N/2 - 1)
 * x[2n + 1] = Im(z[n]), (n = 0, ..., N/2 - 1)
 */

/**
 * This function is the first permutation of two-for-one IFFT algorithm.
 * We move the division by 2 to the last step in the implementation, so:
 * A[k] = (X[k] + X'[N/2 - k])
 * B[k] = (X[k] - X'[N/2 - k]) * W[k], (k = 0, ..., N/2 - 1)
 * Z[k] = (A[k] + j * B[k]) / 2, (k = 0, ..., N/2 - 1)
 */
static void RevbinPermuteInv(const OMX_F32 *in,
                             OMX_F32 *out,
                             const OMX_F32 *twiddle,
                             OMX_INT n) {
  OMX_INT i;
  OMX_INT j;
  OMX_INT i_by_2;
  OMX_INT j_by_2;
  OMX_INT n_by_2 = n >> 1;
  OMX_INT n_by_4 = n >> 2;

  OMX_FC32 big_a;
  OMX_FC32 big_b;
  OMX_FC32 temp;
  const OMX_F32 *tw;

  for (i = 2, j = n - 2; i < n_by_2; i += 2, j -= 2) {
    // A[k] = (X[k] + X'[N/2 - k])
    big_a.Re = in[i] + in[j];
    big_a.Im = in[i + 1] - in[j + 1];

    // temp = (X[k] - X'[N/2 - k])
    temp.Re = in[i] - in[j];
    temp.Im = in[i + 1] + in[j + 1];

    i_by_2 = i >> 1;
    j_by_2 = j >> 1;

    // W[k]
    tw = twiddle + i_by_2;

    // B[k] = (X[k] - X'[N/2 - k]) * W[k]
    big_b.Re =  temp.Re * tw[0] + temp.Im * tw[n];
    big_b.Im =  temp.Re * tw[n] - temp.Im * tw[0];

    // Convert split format to interleaved format.
    // Z[k] = (A[k] + j * B[k]) (k = 0, ..., N/2 - 1)
    // The scaling of 1/2 will be merged into to the scaling in
    // the last step before the output in omxSP_FFTInv_CCSToR_F32_Sfs.
    out[i_by_2] = big_a.Re + big_b.Im;
    out[i_by_2 + n_by_2] = big_b.Re + big_a.Im;
    out[j_by_2] = big_a.Re - big_b.Im;
    out[j_by_2 + n_by_2] = big_b.Re - big_a.Im;
  }

  // The n_by_2 complex point
  out[n_by_4] = 2.0f * in[n_by_2];
  out[n_by_4 + n_by_2] = -2.0f * in[n_by_2 + 1];

  // The first complex point
  out[0] = in[0] + in[n];
  out[n_by_2] = in[0] - in[n];
}

// Sse version of RevbinPermuteInv function.
static void RevbinPermuteInvSse(const OMX_F32 *in,
                                OMX_F32 *out,
                                const OMX_F32 *twiddle,
                                OMX_INT n) {
  OMX_INT i;
  OMX_INT j;
  OMX_INT n_by_2 = n >> 1;
  OMX_INT n_by_4 = n >> 2;
  const OMX_F32 *tw;
  const OMX_F32 *pi;
  const OMX_F32 *pj;

  VC v_i;
  VC v_j;
  VC v_big_a;
  VC v_big_b;
  VC v_temp;
  VC v_tw;

  for (i = 0, j = n_by_2 - 3; i < n_by_4; i += 4, j -= 4) {
    pi = in + (i << 1);
    pj = in + (j << 1);
    VC_LOAD_INTERLEAVE(&v_i, pi);

    v_j.real = _mm_set_ps(pj[0], pj[2], pj[4], pj[6]);
    v_j.imag = _mm_set_ps(pj[1], pj[3], pj[5], pj[7]);

    // A[k] = (X[k] + X'[N/2 - k])
    VC_ADD_SUB(&v_big_a, &v_i, &v_j);

    // temp = (X[k] - X'[N/2 - k])
    VC_SUB_ADD(&v_temp, &v_i, &v_j);

    // W[k]
    tw = twiddle + i;
    VC_LOAD_SPLIT(&v_tw, tw, n);

    // B[k] = (X[k] - X'[N/2 - k]) * W[k]
    VC_CONJ_MUL(&v_big_b, &v_temp, &v_tw);

    // Convert split format to interleaved format.
    // Z[k] = (A[k] + j * B[k]) (k = 0, ..., N/2 - 1)
    // The scaling of 1/2 will be merged into to the scaling in
    // the last step before the output in omxSP_FFTInv_CCSToR_F32_Sfs.
    VC_ADD_X_STORE_SPLIT((out + i), &v_big_a, &v_big_b, n_by_2);

    VC_SUB_X_INVERSE_STOREU_SPLIT((out + j), &v_big_a, &v_big_b, n_by_2);
  }

  // The n_by_2 complex point
  out[n_by_4] = 2.0f * in[n_by_2];
  out[n_by_4 + n_by_2] = -2.0f * in[n_by_2 + 1];

  // The first complex point
  out[0] = in[0] + in[n];
  out[n_by_2] = in[0] - in[n];
}

OMXResult omxSP_FFTInv_CCSToR_F32_Sfs(const OMX_F32 *pSrc, OMX_F32 *pDst,
                                      const OMXFFTSpec_R_F32 *pFFTSpec) {
  OMX_INT n;
  OMX_INT n_by_2;
  OMX_INT i;
  const OMX_F32 *twiddle;
  OMX_F32 *buf;
  OMX_F32 *in = (OMX_F32*) pSrc;

  const X86FFTSpec_R_FC32 *pFFTStruct = (const X86FFTSpec_R_FC32*) pFFTSpec;

  // Input must be 32 byte aligned
  if (!pSrc || !pDst || (const uintptr_t)pSrc & 31 || (uintptr_t)pDst & 31)
    return OMX_Sts_BadArgErr;

  n = pFFTStruct->N;

  // This is to handle the case of order == 1.
  if (n == 2) {
    pDst[0] = (pSrc[0] + pSrc[2]) / 2;
    pDst[1] = (pSrc[0] - pSrc[2]) / 2;
    return OMX_Sts_NoErr;
  }

  n_by_2 = n >> 1;
  buf = pFFTStruct->pBuf1;

  twiddle = pFFTStruct->pTwiddle;

  if (n < 8)
    RevbinPermuteInv(in, buf, twiddle, n);
  else
    RevbinPermuteInvSse(in, buf, twiddle, n);

  if (n_by_2 < 16) {
    buf = x86SP_F32_radix2_kernel_OutOfPlace(
        buf,
        pFFTStruct->pBuf2,
        buf,
        twiddle,
        n_by_2,
        0);
  } else {
    buf = x86SP_F32_radix4_kernel_OutOfPlace_sse(
        buf,
        pFFTStruct->pBuf2,
        buf,
        twiddle,
        n_by_2,
        0);
  }

  // Scale the result by 1/n.
  // It contains a scaling factor of 1/2 in
  // RevbinPermuteInv/RevbinPermuteInvSse.
  OMX_F32 factor = 1.0f / n;

  if (n < 8) {
    for (i = 0; i < n_by_2; i++) {
      pDst[i << 1] = buf[i] * factor;
      pDst[(i << 1) + 1] = buf[i + n_by_2] * factor;
    }
  } else {
    OMX_F32 *base;
    OMX_F32 *dst;
    VC temp0;
    VC temp1;
    __m128 mFactor = _mm_load1_ps(&factor);

    // Two things are done in this loop:
    // 1 Get the result scaled; 2 Change the format from split to interleaved.
    for (i = 0; i < n_by_2; i += 4) {
      base = buf + i;
      dst = pDst + (i << 1);
      VC_LOAD_SPLIT(&temp0, base, n_by_2);
      VC_MUL_F(&temp1, &temp0, mFactor);
      VC_STORE_INTERLEAVE(dst, &temp1);
    }
  }

  return OMX_Sts_NoErr;
}
