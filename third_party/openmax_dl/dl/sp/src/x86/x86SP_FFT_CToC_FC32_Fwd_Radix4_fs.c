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

void x86SP_FFT_CToC_FC32_Fwd_Radix4_fs(
    const OMX_F32 *in,
    OMX_F32 *out,
    OMX_INT n) {
  OMX_INT i;
  OMX_INT n_by_4 = n >> 2;

  // Transform from interleaved format to split format.
  for (i = 0; i < n; i++) {
    out[i] = in[i << 1];
    out[i + n] = in[(i << 1) + 1];
  }

  // As we have already moved data from [in] to [out],
  // next calculation will be produced in in-place mode.
  for (i = 0; i < n_by_4; i++) {
    OMX_F32 *out0 = out + i;
    OMX_F32 *out1 = out0 + n_by_4;
    OMX_F32 *out2 = out1 + n_by_4;
    OMX_F32 *out3 = out2 + n_by_4;

    OMX_FC32 t0;
    OMX_FC32 t1;
    OMX_FC32 t2;
    OMX_FC32 t3;

    // CADD t0, out0, out2
    t0.Re = out0[0] + out2[0];
    t0.Im = out0[n] + out2[n];

    // CSUB t1, out0, out2
    t1.Re = out0[0] - out2[0];
    t1.Im = out0[n] - out2[n];

    // CADD t2, out1, out3
    t2.Re = out1[0] + out3[0];
    t2.Im = out1[n] + out3[n];

    // CSUB t3, out1, out3
    t3.Re = out1[0] - out3[0];
    t3.Im = out1[n] - out3[n];

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
  }
}
