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
 * A two-for-one algorithm is used here to do the real fft:
 *
 * Input x[n], (n = 0, ..., N - 1)
 * Output X[k] = DFT(N, k){x}
 * a[n] = x[2n], (n = 0, ..., N/2 - 1)
 * b[n] = x[2n + 1], (n = 0, ..., N/2 - 1)
 * z[n] = a[n] + j * b[n]
 * Z[k] = DFT(N/2, k){z}
 * Z' is the complex conjugate of Z
 * A[k] = (Z[k] + Z'[N/2 - k]) / 2
 * B[k] = -j * (Z[k] - Z'[N/2 - k]) / 2
 * X[k] = A[k] + B[k] * W[k], (W = exp(-j*2*PI*k/N); k = 0, ..., N/2 - 1)
 * X[k] = A[k] - B[k], (k = N/2)
 * X' is complex conjugate of X
 * X[k] = X'[N - k], (k = N/2 + 1, ..., N - 1)
 */

/**
 * This function is the last permutation of two-for-one FFT algorithm.
 * We move the division by 2 to the last step in the implementation, so:
 * A[k] = (Z[k] + Z'[N/2 - k])
 * B[k] = -j * (Z[k] - Z'[N/2 - k])
 * X[k] = (A[k] + B[k] * W[k]) / 2, (k = 0, ..., N/2 - 1)
 * X[k] = (A[k] - B[k]), (k = N/2)
 * X[k] = X'[N - k], (k = N/2 + 1, ..., N - 1)
 */
static void RevbinPermuteFwd(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n) {
  OMX_INT i;
  OMX_INT j;
  OMX_INT n_by_2 = n >> 1;
  OMX_INT n_by_4 = n >> 2;

  OMX_FC32 big_a;
  OMX_FC32 big_b;
  OMX_FC32 temp;
  const OMX_F32 *tw;

  for (i = 1, j = n_by_2 - 1; i < n_by_4; i++, j--) {
    // A[k] = (Z[k] + Z'[N/2 - k])
    big_a.Re = in[i] + in[j];
    big_a.Im = in[j + n_by_2] - in[i + n_by_2];

    // B[k] = -j * (Z[k] - Z'[N/2 - k])
    big_b.Re = in[j] - in[i];
    big_b.Im = in[j + n_by_2] + in[i + n_by_2];

    // W[k]
    tw = twiddle + i;

    // temp = B[k] * W[k]
    temp.Re =  big_b.Re * tw[0] + big_b.Im * tw[n];
    temp.Im =  big_b.Re * tw[n] - big_b.Im * tw[0];

    // Convert split format to interleaved format.
    // X[k] = (A[k] + B[k] * W[k]) / 2, (k = 0, ..., N/2 - 1)
    out[i << 1] = 0.5f * (big_a.Re - temp.Im);
    out[(i << 1) + 1] = 0.5f * (temp.Re - big_a.Im);
    // X[k] = X'[N - k] (k = N/2 + 1, ..., N - 1)
    out[j << 1] = 0.5f * (big_a.Re + temp.Im);
    out[(j << 1) + 1] = 0.5f * (temp.Re + big_a.Im);
  }

  // X[k] = A[k] - B[k] (k = N/2)
  out[n_by_2] = in[n_by_4];
  out[n_by_2 + 1] = -in[n_by_4 + n_by_2];

  out[0] = in[0] + in[n_by_2];
  out[1] = 0;
  out[n] = in[0] - in[n_by_2];
  out[n + 1] = 0;
}

// Sse version of RevbinPermuteFwd function.
static void RevbinPermuteFwdSse(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n) {
  OMX_INT i;
  OMX_INT j;
  OMX_INT n_by_2 = n >> 1;
  OMX_INT n_by_4 = n >> 2;

  VC v_i;
  VC v_j;
  VC v_big_a;
  VC v_big_b;
  VC v_temp;
  VC v_x0;
  VC v_x1;
  VC v_tw;

  __m128 factor = _mm_set1_ps(0.5f);

  for (i = 0, j = n_by_2 - 3; i < n_by_4; i += 4, j -= 4) {
    VC_LOAD_SPLIT(&v_i, (in + i), n_by_2);

    VC_LOADU_SPLIT(&v_j, (in + j), n_by_2);
    VC_REVERSE(&v_j);

    // A[k] = (Z[k] + Z'[N/2 - k])
    VC_ADD_SUB(&v_big_a, &v_j, &v_i);

    // B[k] = -j * (Z[k] - Z'[N/2 - k])
    VC_SUB_ADD(&v_big_b, &v_j, &v_i);

    // W[k]
    VC_LOAD_SPLIT(&v_tw, (twiddle + i), n);

    // temp = B[k] * W[k]
    VC_CONJ_MUL(&v_temp, &v_big_b, &v_tw);

    VC_SUB_X(&v_x0, &v_big_a, &v_temp);
    VC_ADD_X(&v_x1, &v_big_a, &v_temp);

    VC_MUL_F(&v_x0, &v_x0, factor);
    VC_MUL_F(&v_x1, &v_x1, factor);

    // X[k] = A[k] + B[k] * W[k] (k = 0, ..., N/2 - 1)
    VC_STORE_INTERLEAVE((out + (i << 1)), &v_x0);

    // X[k] = X'[N - k] (k = N/2 + 1, ..., N - 1)
    VC_REVERSE(&v_x1);
    VC_STOREU_INTERLEAVE((out + (j << 1)), &v_x1);
  }

  out[n_by_2] = in[n_by_4];
  out[n_by_2 + 1] = -in[n_by_4 + n_by_2];

  out[0] = in[0] + in[n_by_2];
  out[1] = 0;
  out[n] = in[0] - in[n_by_2];
  out[n + 1] = 0;
}

OMXResult omxSP_FFTFwd_RToCCS_F32_Sfs(const OMX_F32 *pSrc, OMX_F32 *pDst,
                                      const OMXFFTSpec_R_F32 *pFFTSpec) {
  OMX_INT n;
  OMX_INT n_by_2;
  const OMX_F32 *twiddle;
  OMX_F32 *buf;

  const X86FFTSpec_R_FC32 *pFFTStruct = (const X86FFTSpec_R_FC32*) pFFTSpec;

  // Input must be 32 byte aligned
  if (!pSrc || !pDst || (const uintptr_t)pSrc & 31 || (uintptr_t)pDst & 31)
    return OMX_Sts_BadArgErr;

  n = pFFTStruct->N;

  // This is to handle the case of order == 1.
  if (n == 2) {
    pDst[0] = (pSrc[0] + pSrc[1]);
    pDst[1] = 0.0f;
    pDst[2] = (pSrc[0] - pSrc[1]);
    pDst[3] = 0.0f;
    return OMX_Sts_NoErr;
  }

  n_by_2 = n >> 1;
  buf = pFFTStruct->pBuf1;
  twiddle = pFFTStruct->pTwiddle;

  if(n_by_2 >= 16) {
    buf = x86SP_F32_radix4_kernel_OutOfPlace_sse(
        pSrc,
        pFFTStruct->pBuf2,
        buf,
        twiddle,
        n_by_2,
        1);
  } else {
    buf = x86SP_F32_radix2_kernel_OutOfPlace(
        pSrc,
        pFFTStruct->pBuf2,
        buf,
        twiddle,
        n_by_2,
        1);
  }

  if(n >= 8)
    RevbinPermuteFwdSse(buf, pDst, twiddle, n);
  else
    RevbinPermuteFwd(buf, pDst, twiddle, n);

  return OMX_Sts_NoErr;
}
