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

void x86SP_FFT_CToC_FC32_Fwd_Radix4_fs_sse(
    const OMX_F32 *in,
    OMX_F32 *out,
    OMX_INT n) {
  OMX_INT i;
  OMX_INT n_by_2 = n >> 1;
  OMX_INT n_by_4 = n >> 2;
  OMX_F32 *out0 = out;

  for (i = 0; i < n_by_2; i += 8) {
    VC v_t0;
    VC v_t1;
    VC v_t2;
    VC v_t3;
    VC v_t4;
    VC v_t5;
    VC v_t6;
    VC v_t7;

    const OMX_F32 *in0 = in + i;
    const OMX_F32 *in1 = in0 + n_by_2;
    const OMX_F32 *in2 = in1 + n_by_2;
    const OMX_F32 *in3 = in2 + n_by_2;

    OMX_F32 *out1 = out0 + n_by_4;
    OMX_F32 *out2 = out1 + n_by_4;
    OMX_F32 *out3 = out2 + n_by_4;

    VC_LOAD_SHUFFLE(&(v_t0.real), &(v_t0.imag), in0);
    VC_LOAD_SHUFFLE(&(v_t1.real), &(v_t1.imag), in1);
    VC_LOAD_SHUFFLE(&(v_t2.real), &(v_t2.imag), in2);
    VC_LOAD_SHUFFLE(&(v_t3.real), &(v_t3.imag), in3);

    RADIX4_BUTTERFLY_FS(&v_t4, &v_t5, &v_t6, &v_t7,
                        &v_t0, &v_t1, &v_t2, &v_t3);

    RADIX4_FWD_BUTTERFLY_STORE(out0, out1, out2, out3,
                               &v_t4, &v_t5, &v_t6, &v_t7, n);

    out0 += 4;
  }
}
