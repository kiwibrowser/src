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

void x86SP_FFT_CToC_FC32_Inv_Radix4_fs(
    const OMX_F32 *in,
    OMX_F32 *out,
    OMX_INT n) {
  OMX_INT i;
  OMX_INT n_by_4 = n >> 2;
  OMX_F32 *out0 = out;

  for (i = 0; i < n_by_4; i++) {
    const OMX_F32 *in0 = in + i;
    const OMX_F32 *in1 = in0 + n_by_4;
    const OMX_F32 *in2 = in1 + n_by_4;
    const OMX_F32 *in3 = in2 + n_by_4;
    OMX_F32 *out1 = out0 + n_by_4;
    OMX_F32 *out2 = out1 + n_by_4;
    OMX_F32 *out3 = out2 + n_by_4;

    OMX_FC32 t0;
    OMX_FC32 t1;
    OMX_FC32 t2;
    OMX_FC32 t3;

    // CADD t0, in0, in2
    t0.Re = in0[0] + in2[0];
    t0.Im = in0[n] + in2[n];

    // CSUB t1, in0, in2
    t1.Re = in0[0] - in2[0];
    t1.Im = in0[n] - in2[n];

    // CADD t2, in1, in3
    t2.Re = in1[0] + in3[0];
    t2.Im = in1[n] + in3[n];

    // CSUB t3, in1, in3
    t3.Re = in1[0] - in3[0];
    t3.Im = in1[n] - in3[n];

    // CADD out0, t0, t2
    out0[0] = t0.Re + t2.Re;
    out0[n] = t0.Im + t2.Im;

    // CSUB out2, t0, t2
    out2[0] = t0.Re - t2.Re;
    out2[n] = t0.Im - t2.Im;

    // CSUB_ADD_X out1, t1, t3
    out1[0] = t1.Re - t3.Im;
    out1[n] = t1.Im + t3.Re;

    // CADD_SUB_X out3, t1, t3
    out3[0] = t1.Re + t3.Im;
    out3[n] = t1.Im - t3.Re;

    out0 += 1;
  }
}
