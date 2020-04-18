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

void x86SP_FFT_CToC_FC32_Fwd_Radix2_fs(
    const OMX_F32 *in,
    OMX_F32 *out,
    OMX_INT n) {
  OMX_INT i;
  OMX_F32 *out0 = out;

  for (i = 0; i < n; i += 2) {
    const OMX_F32 *in0 = in + i;
    const OMX_F32 *in1 = in0 + n;
    OMX_F32 *out1 = out0 + (n >> 1);

    // CADD out0, in0, in1
    out0[0] = in0[0] + in1[0];
    out0[n] = in0[1] + in1[1];

    // CSUB out1, in0, in1
    out1[0] = in0[0] - in1[0];
    out1[n] = in0[1] - in1[1];

    out0 += 1;
  }
}
