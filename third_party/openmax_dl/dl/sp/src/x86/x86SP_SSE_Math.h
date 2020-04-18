/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights realserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#include <emmintrin.h>
#include <assert.h>

/**
 * Two data formats are used by the FFT routines, internally. The
 * interface to the main external FFT routines use interleaved complex
 * values where the real part is followed by the imaginary part.
 *
 * One is the split format where a complex vector of real and imaginary
 * values are split such that all of the real values are placed in the
 * first half of the vector and the corresponding values are placed in
 * the second half, in the same order. The conversion from interleaved
 * complex values to split format and back is transparent to the
 * external FFT interface.
 *
 * VComplex uses split format.
 */

/** VComplex hold 4 complex float elements, with the real parts stored
 * in real and corresponding imaginary parts in imag.
 */
typedef struct VComplex {
  __m128 real;
  __m128 imag;
} VC;

/* out = a * b */
static __inline void VC_MUL(VC *out, VC *a, VC *b) {
  out->real = _mm_sub_ps(_mm_mul_ps(a->real, b->real),
      _mm_mul_ps(a->imag, b->imag));
  out->imag = _mm_add_ps(_mm_mul_ps(a->real, b->imag),
      _mm_mul_ps(a->imag, b->real));
}

/* out = conj(a) * b */
static __inline void VC_CONJ_MUL(VC *out, VC *a, VC *b) {
  out->real = _mm_add_ps(_mm_mul_ps(a->real, b->real),
      _mm_mul_ps(a->imag, b->imag));
  out->imag = _mm_sub_ps(_mm_mul_ps(a->real, b->imag),
      _mm_mul_ps(a->imag, b->real));
}

/* Scale complex by a real factor */
static __inline void VC_MUL_F(VC *out, VC *a, __m128 factor) {
  out->real = _mm_mul_ps(factor, a->real);
  out->imag = _mm_mul_ps(factor, a->imag);
}

/* out = a + b */
static __inline void VC_ADD(VC *out, VC *a, VC *b) {
  out->real = _mm_add_ps(a->real, b->real);
  out->imag = _mm_add_ps(a->imag, b->imag);
}

/**
 * out.real = a.real + b.imag
 * out.imag = a.imag + b.real
 */
static __inline void VC_ADD_X(VC *out, VC *a, VC *b) {
  out->real = _mm_add_ps(a->real, b->imag);
  out->imag = _mm_add_ps(b->real, a->imag);
}

/* VC_ADD and store the result with Split format. */
static __inline void VC_ADD_STORE_SPLIT(
    OMX_F32 *out,
    VC *a,
    VC *b,
    OMX_INT offset) {
  _mm_store_ps(out, _mm_add_ps(a->real, b->real));
  _mm_store_ps(out + offset, _mm_add_ps(a->imag, b->imag));
}

/* out = a - b */
static __inline void VC_SUB(VC *out, VC *a, VC *b) {
  out->real = _mm_sub_ps(a->real, b->real);
  out->imag = _mm_sub_ps(a->imag, b->imag);
}

/**
 * out.real = a.real - b.imag
 * out.imag = a.imag - b.real
 */
static __inline void VC_SUB_X(VC *out, VC *a, VC *b) {
  out->real = _mm_sub_ps(a->real, b->imag);
  out->imag = _mm_sub_ps(b->real, a->imag);
}

/* VC_SUB and store the result with Split format. */
static __inline void VC_SUB_STORE_SPLIT(
    OMX_F32 *out,
    VC *a,
    VC *b,
    OMX_INT offset) {
  _mm_store_ps(out, _mm_sub_ps(a->real, b->real));
  _mm_store_ps(out + offset, _mm_sub_ps(a->imag, b->imag));
}

/**
 * out.real = a.real + b.real
 * out.imag = a.imag - b.imag
 */
static __inline void VC_ADD_SUB(VC *out, VC *a, VC *b) {
  out->real = _mm_add_ps(a->real, b->real);
  out->imag = _mm_sub_ps(a->imag, b->imag);
}

/**
 * out.real = a.real + b.imag
 * out.imag = a.imag - b.real
 */
static __inline void VC_ADD_SUB_X(VC *out, VC *a, VC *b) {
  out->real = _mm_add_ps(a->real, b->imag);
  out->imag = _mm_sub_ps(a->imag, b->real);
}

/* VC_ADD_SUB_X and store the result with Split format. */
static __inline void VC_ADD_SUB_X_STORE_SPLIT(
    OMX_F32 *out,
    VC *a,
    VC *b,
    OMX_INT offset) {
  _mm_store_ps(out, _mm_add_ps(a->real, b->imag));
  _mm_store_ps(out + offset, _mm_sub_ps(a->imag, b->real));
}

/**
 * out.real = a.real - b.real
 * out.imag = a.imag + b.imag
 */
static __inline void VC_SUB_ADD(VC *out, VC *a, VC *b) {
  out->real = _mm_sub_ps(a->real, b->real);
  out->imag = _mm_add_ps(a->imag, b->imag);
}

/**
 * out.real = a.real - b.imag
 * out.imag = a.imag + b.real
 */
static __inline void VC_SUB_ADD_X(VC *out, VC *a, VC *b) {
  out->real = _mm_sub_ps(a->real, b->imag);
  out->imag = _mm_add_ps(a->imag, b->real);
}

/* VC_SUB_ADD_X and store the result with Split format. */
static __inline void VC_SUB_ADD_X_STORE_SPLIT(
    OMX_F32 *out,
    VC *a, VC *b,
    OMX_INT offset) {
  _mm_store_ps(out, _mm_sub_ps(a->real, b->imag));
  _mm_store_ps(out + offset, _mm_add_ps(a->imag, b->real));
}

/**
 * out[0]      = in.real
 * out[offset] = in.imag
 */
static __inline void VC_STORE_SPLIT(
    OMX_F32 *out,
    VC *in,
    OMX_INT offset) {
  _mm_store_ps(out, in->real);
  _mm_store_ps(out + offset, in->imag);
}

/**
 * out.real = in[0];
 * out.imag = in[offset];
*/
static __inline void VC_LOAD_SPLIT(
    VC *out,
    const OMX_F32 *in,
    OMX_INT offset) {
  out->real = _mm_load_ps(in);
  out->imag = _mm_load_ps(in + offset);
}

/* Vector Complex Unpack from Split format to Interleaved format. */
static __inline void VC_UNPACK(VC *out, VC *in) {
    out->real = _mm_unpacklo_ps(in->real, in->imag);
    out->imag = _mm_unpackhi_ps(in->real, in->imag);
}

/**
 * Vector Complex load from interleaved complex array.
 * out.real = [in[0].real, in[1].real, in[2].real, in[3].real]
 * out.imag = [in[0].imag, in[1].imag, in[2].imag, in[3].imag]
 */
static __inline void VC_LOAD_INTERLEAVE(VC *out, const OMX_F32 *in) {
    __m128 temp0 = _mm_load_ps(in);
    __m128 temp1 = _mm_load_ps(in + 4);
    out->real = _mm_shuffle_ps(temp0, temp1, _MM_SHUFFLE(2, 0, 2, 0));
    out->imag = _mm_shuffle_ps(temp0, temp1, _MM_SHUFFLE(3, 1, 3, 1));
}
/**
 * Vector Complex Load with Split format.
 * The input address is not 16 byte aligned.
 */
static __inline void VC_LOADU_SPLIT(
    VC *out,
    const OMX_F32 *in,
    OMX_INT offset) {
  out->real = _mm_loadu_ps(in);
  out->imag = _mm_loadu_ps(in + offset);
}

/* Reverse the order of the Complex Vector. */
static __inline void VC_REVERSE(VC *v) {
  v->real = _mm_shuffle_ps(v->real, v->real, _MM_SHUFFLE(0, 1, 2, 3));
  v->imag = _mm_shuffle_ps(v->imag, v->imag, _MM_SHUFFLE(0, 1, 2, 3));
}
/*
 * Vector Complex store to interleaved complex array
 * out[0] = in.real[0]
 * out[1] = in.imag[0]
 * out[2] = in.real[1]
 * out[3] = in.imag[1]
 * out[4] = in.real[2]
 * out[5] = in.imag[2]
 * out[6] = in.real[3]
 * out[7] = in.imag[3]
 */
static __inline void VC_STORE_INTERLEAVE(OMX_F32 *out, VC *in) {
  _mm_store_ps(out, _mm_unpacklo_ps(in->real, in->imag));
  _mm_store_ps(out + 4, _mm_unpackhi_ps(in->real, in->imag));
}

/**
 * Vector Complex Store with Interleaved format.
 * Address is not 16 byte aligned.
 */
static __inline void VC_STOREU_INTERLEAVE(OMX_F32 *out, VC *in) {
  _mm_storeu_ps(out, _mm_unpacklo_ps(in->real, in->imag));
  _mm_storeu_ps(out + 4, _mm_unpackhi_ps(in->real, in->imag));
}

/* VC_ADD_X and store the result with Split format. */
static __inline void VC_ADD_X_STORE_SPLIT(
    OMX_F32 *out,
    VC *a, VC *b,
    OMX_INT offset) {
  _mm_store_ps(out, _mm_add_ps(a->real, b->imag));
  _mm_store_ps(out + offset, _mm_add_ps(b->real, a->imag));
}

/**
 * VC_SUB_X and store the result with inverse order.
 * Address is not 16 byte aligned.
 */
static __inline void VC_SUB_X_INVERSE_STOREU_SPLIT(
    OMX_F32 *out,
    VC *a,
    VC *b,
    OMX_INT offset) {
  __m128 t;
  t = _mm_sub_ps(a->real, b->imag);
  _mm_storeu_ps(out, _mm_shuffle_ps(t, t, _MM_SHUFFLE(0, 1, 2, 3)));
  t = _mm_sub_ps(b->real, a->imag);
  _mm_storeu_ps(out + offset, _mm_shuffle_ps(t, t, _MM_SHUFFLE(0, 1, 2, 3)));
}

/**
 * Vector Complex Load from Interleaved format to Split format.
 * Store the result into two __m128 registers.
 */
static __inline void VC_LOAD_SHUFFLE(
    __m128 *out0,
    __m128 *out1,
    const OMX_F32 *in) {
  VC temp;
  VC_LOAD_INTERLEAVE(&temp, in);
  *out0 = temp.real;
  *out1 = temp.imag;
}

/* Finish the butterfly calculation of forward radix4 and store the outputs. */
static __inline void RADIX4_FWD_BUTTERFLY_STORE(
    OMX_F32 *out0,
    OMX_F32 *out1,
    OMX_F32 *out2,
    OMX_F32 *out3,
    VC *t0,
    VC *t1,
    VC *t2,
    VC *t3,
    OMX_INT n) {
  /* CADD out0, t0, t2 */
  VC_ADD_STORE_SPLIT(out0, t0, t2, n);

  /* CSUB out2, t0, t2 */
  VC_SUB_STORE_SPLIT(out2, t0, t2, n);

  /* CADD_SUB_X out1, t1, t3 */
  VC_ADD_SUB_X_STORE_SPLIT(out1, t1, t3, n);

  /* CSUB_ADD_X out3, t1, t3 */
  VC_SUB_ADD_X_STORE_SPLIT(out3, t1, t3, n);
}

/* Finish the butterfly calculation of inverse radix4 and store the outputs. */
static __inline void RADIX4_INV_BUTTERFLY_STORE(
    OMX_F32 *out0,
    OMX_F32 *out1,
    OMX_F32 *out2,
    OMX_F32 *out3,
    VC *t0,
    VC *t1,
    VC *t2,
    VC *t3,
    OMX_INT n) {
  /* CADD out0, t0, t2 */
  VC_ADD_STORE_SPLIT(out0, t0, t2, n);

  /* CSUB out2, t0, t2 */
  VC_SUB_STORE_SPLIT(out2, t0, t2, n);

  /* CSUB_ADD_X out1, t1, t3 */
  VC_SUB_ADD_X_STORE_SPLIT(out1, t1, t3, n);

  /* CADD_SUB_X out3, t1, t3 */
  VC_ADD_SUB_X_STORE_SPLIT(out3, t1, t3, n);
}

/* Radix4 forward butterfly */
static __inline void RADIX4_FWD_BUTTERFLY(
    VC *t0,
    VC *t1,
    VC *t2,
    VC *t3,
    VC *Tw1,
    VC *Tw2,
    VC *Tw3,
    VC *T0,
    VC *T1,
    VC *T2,
    VC *T3) {
  VC tt1, tt2, tt3;

  /* CMUL tt1, Tw1, T1 */
  VC_MUL(&tt1, Tw1, T1);

  /* CMUL tt2, Tw2, T2 */
  VC_MUL(&tt2, Tw2, T2);

  /* CMUL tt3, Tw3, T3 */
  VC_MUL(&tt3, Tw3, T3);

  /* CADD t0, T0, tt2 */
  VC_ADD(t0, T0, &tt2);

  /* CSUB t1, T0, tt2 */
  VC_SUB(t1, T0, &tt2);

  /* CADD t2, tt1, tt3 */
  VC_ADD(t2, &tt1, &tt3);

  /* CSUB t3, tt1, tt3 */
  VC_SUB(t3, &tt1, &tt3);
}

/* Radix4 inverse butterfly */
static __inline void RADIX4_INV_BUTTERFLY(
    VC *t0,
    VC *t1,
    VC *t2,
    VC *t3,
    VC *Tw1,
    VC *Tw2,
    VC *Tw3,
    VC *T0,
    VC *T1,
    VC *T2,
    VC *T3) {
  VC tt1, tt2, tt3;

  /* CMUL tt1, Tw1, T1 */
  VC_CONJ_MUL(&tt1, Tw1, T1);

  /* CMUL tt2, Tw2, T2 */
  VC_CONJ_MUL(&tt2, Tw2, T2);

  /* CMUL tt3, Tw3, T3 */
  VC_CONJ_MUL(&tt3, Tw3, T3);

  /* CADD t0, T0, tt2 */
  VC_ADD(t0, T0, &tt2);

  /* CSUB t1, T0, tt2 */
  VC_SUB(t1, T0, &tt2);

  /* CADD t2, tt1, tt3 */
  VC_ADD(t2, &tt1, &tt3);

  /* CSUB t3, tt1, tt3 */
  VC_SUB(t3, &tt1, &tt3);
}

/* Radix4 butterfly in first stage for both forward and inverse */
static __inline void RADIX4_BUTTERFLY_FS(
    VC *t0,
    VC *t1,
    VC *t2,
    VC *t3,
    VC *T0,
    VC *T1,
    VC *T2,
    VC *T3) {
  /* CADD t0, T0, T2 */
  VC_ADD(t0, T0, T2);

  /* CSUB t1, T0, T2 */
  VC_SUB(t1, T0, T2);

  /* CADD t2, T1, T3 */
  VC_ADD(t2, T1, T3);

  /* CSUB t3, T1, T3 */
  VC_SUB(t3, T1, T3);
}

/**
 * Load 16 float elements (4 sse registers) which is a 4 * 4 matrix.
 * Then Do transpose on the matrix.
 * 3,  2,  1,  0                  12, 8,  4,  0
 * 7,  6,  5,  4        =====>    13, 9,  5,  1
 * 11, 10, 9,  8                  14, 10, 6,  2
 * 15, 14, 13, 12                 15, 11, 7,  3
 */
static __inline void VC_LOAD_MATRIX_TRANSPOSE(
    VC *T0,
    VC *T1,
    VC *T2,
    VC *T3,
    const OMX_F32 *pT0,
    const OMX_F32 *pT1,
    const OMX_F32 *pT2,
    const OMX_F32 *pT3,
    OMX_INT n) {
  __m128 xmm0;
  __m128 xmm1;
  __m128 xmm2;
  __m128 xmm3;
  __m128 xmm4;
  __m128 xmm5;
  __m128 xmm6;
  __m128 xmm7;

  xmm0 = _mm_load_ps(pT0);
  xmm1 = _mm_load_ps(pT1);
  xmm2 = _mm_load_ps(pT2);
  xmm3 = _mm_load_ps(pT3);

  /* Matrix transpose */
  xmm4 = _mm_unpacklo_ps(xmm0, xmm1);
  xmm5 = _mm_unpackhi_ps(xmm0, xmm1);
  xmm6 = _mm_unpacklo_ps(xmm2, xmm3);
  xmm7 = _mm_unpackhi_ps(xmm2, xmm3);
  T0->real = _mm_shuffle_ps(xmm4, xmm6, _MM_SHUFFLE(1, 0, 1, 0));
  T1->real = _mm_shuffle_ps(xmm4, xmm6, _MM_SHUFFLE(3, 2, 3, 2));
  T2->real = _mm_shuffle_ps(xmm5, xmm7, _MM_SHUFFLE(1, 0, 1, 0));
  T3->real = _mm_shuffle_ps(xmm5, xmm7, _MM_SHUFFLE(3, 2, 3, 2));

  xmm0 = _mm_load_ps(pT0 + n);
  xmm1 = _mm_load_ps(pT1 + n);
  xmm2 = _mm_load_ps(pT2 + n);
  xmm3 = _mm_load_ps(pT3 + n);

  /* Matrix transpose */
  xmm4 = _mm_unpacklo_ps(xmm0, xmm1);
  xmm5 = _mm_unpackhi_ps(xmm0, xmm1);
  xmm6 = _mm_unpacklo_ps(xmm2, xmm3);
  xmm7 = _mm_unpackhi_ps(xmm2, xmm3);
  T0->imag = _mm_shuffle_ps(xmm4, xmm6, _MM_SHUFFLE(1, 0, 1, 0));
  T1->imag = _mm_shuffle_ps(xmm4, xmm6, _MM_SHUFFLE(3, 2, 3, 2));
  T2->imag = _mm_shuffle_ps(xmm5, xmm7, _MM_SHUFFLE(1, 0, 1, 0));
  T3->imag = _mm_shuffle_ps(xmm5, xmm7, _MM_SHUFFLE(3, 2, 3, 2));
}
