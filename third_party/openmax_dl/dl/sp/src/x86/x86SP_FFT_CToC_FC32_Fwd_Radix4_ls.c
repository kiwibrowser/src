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

void x86SP_FFT_CToC_FC32_Fwd_Radix4_ls(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n) {
  OMX_INT n_by_2 = n >> 1;
  OMX_INT n_by_4 = n >> 2;
  OMX_INT n_mul_2 = n << 1;
  OMX_INT i;
  OMX_F32 *out0 = out;

  for (i = 0; i < n_by_2; i += 2) {
    OMX_FC32 t0;
    OMX_FC32 t1;
    OMX_FC32 t2;
    OMX_FC32 t3;
    OMX_FC32 tt1;
    OMX_FC32 tt2;
    OMX_FC32 tt3;
    const OMX_F32 *tw1 = twiddle + i;
    const OMX_F32 *tw2 = tw1 + i;
    const OMX_F32 *tw3 = tw2 + i;
    const OMX_F32 *in0 = in + (i << 1);
    const OMX_F32 *in1 = in0 + 1;
    const OMX_F32 *in2 = in1 + 1;
    const OMX_F32 *in3 = in2 + 1;
    OMX_F32 *out1 = out0 + n_by_4;
    OMX_F32 *out2 = out1 + n_by_4;
    OMX_F32 *out3 = out2 + n_by_4;

    // CMUL tt1, tw1, in1
    tt1.Re = tw1[0] * in1[0] - tw1[n_mul_2] * in1[n];
    tt1.Im = tw1[0] * in1[n] + tw1[n_mul_2] * in1[0];

    // CMUL tt2, tw2, in2
    tt2.Re = tw2[0] * in2[0] - tw2[n_mul_2] * in2[n];
    tt2.Im = tw2[0] * in2[n] + tw2[n_mul_2] * in2[0];

    // CMUL tt3, tw3, in3
    tt3.Re = tw3[0] * in3[0] - tw3[n_mul_2] * in3[n];
    tt3.Im = tw3[0] * in3[n] + tw3[n_mul_2] * in3[0];

    // CADD t0, in0, tt2
    t0.Re = in0[0] + tt2.Re;
    t0.Im = in0[n] + tt2.Im;

    // CSUB t1, in0, tt2
    t1.Re = in0[0] - tt2.Re;
    t1.Im = in0[n] - tt2.Im;

    // CADD t2, tt1, tt3
    t2.Re = tt1.Re + tt3.Re;
    t2.Im = tt1.Im + tt3.Im;

    // CSUB t3, tt1, tt3
    t3.Re = tt1.Re - tt3.Re;
    t3.Im = tt1.Im - tt3.Im;

    // CADD out0, t0, t2
    out0[0] = t0.Re + t2.Re;
    out0[n] = t0.Im + t2.Im;

    // CSUB out2, t0, t2
    out2[0] = t0.Re - t2.Re;
    out2[n] = t0.Im - t2.Im;

    // CADD_SUB_X out1, t1, t3
    out1[0] = t1.Re + t3.Im;
    out1[n] = t1.Im - t3.Re;

    // CSUB_ADD_X out3, t1, t3
    out3[0] = t1.Re - t3.Im;
    out3[n] = t1.Im + t3.Re;

    out0 += 1;
  }
}
