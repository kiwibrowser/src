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

void x86SP_FFT_CToC_FC32_Fwd_Radix2_ls_sse(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n) {
  OMX_F32 *out0 = out;
  OMX_INT i;

  // This function is used when n >= 8
  assert(n >= 8);
  if (n < 8) return;

  for (i = 0; i < n; i += 8) {
    VC v_tw;
    VC v_t0;
    VC v_t1;
    VC v_temp;

    // Load twiddle
    const OMX_F32 *tw = twiddle + i;
    v_tw.real = _mm_set_ps(tw[6], tw[4], tw[2], tw[0]);
    const OMX_F32 * twi = tw + (n << 1);
    v_tw.imag = _mm_set_ps(twi[6], twi[4], twi[2], twi[0]);

    // Load real part
    const OMX_F32 *t = in + i;
    VC_LOAD_SHUFFLE(&(v_t0.real), &(v_t1.real), t);

    // Load imag part
    t = t + n;
    VC_LOAD_SHUFFLE(&(v_t0.imag), &(v_t1.imag), t);

    OMX_F32 *out1 = out0 + (n >> 1);
    VC_MUL(&v_temp, &v_tw, &v_t1);

    VC_SUB_STORE_SPLIT(out1, &v_t0, &v_temp, n);

    VC_ADD_STORE_SPLIT(out0, &v_t0, &v_temp, n);

    out0 += 4;
  }
}
