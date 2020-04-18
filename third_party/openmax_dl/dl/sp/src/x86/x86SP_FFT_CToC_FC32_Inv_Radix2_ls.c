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

void x86SP_FFT_CToC_FC32_Inv_Radix2_ls(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n) {
  OMX_INT i;
  OMX_F32 *out0 = out;

  for (i = 0; i < n; i += 2) {
    OMX_FC32 t;
    const OMX_F32 *tw = twiddle + i;
    const OMX_F32 *in0 = in + i;
    const OMX_F32 *in1 = in0 + 1;
    OMX_F32 *out1 = out0 + (n >> 1);

    // CMUL t, tw, in1
    t.Re = tw[0] * in1[0] + tw[n << 1] * in1[n];
    t.Im = tw[0] * in1[n] - tw[n << 1] * in1[0];

    // CADD out0, in0, t
    out0[0] = in0[0] + t.Re;
    out0[n] = in0[n] + t.Im;

    // CSUB out1, in0, t
    out1[0] = in0[0] - t.Re;
    out1[n] = in0[n] - t.Im;

    out0 += 1;
  }
}
