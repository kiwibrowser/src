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

void x86SP_FFT_CToC_FC32_Fwd_Radix2_ms(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n,
    OMX_INT sub_size,
    OMX_INT sub_num) {
  OMX_INT grp;
  OMX_F32 *out0 = out;
  OMX_INT set_count = sub_num >> 1;

  for (grp = 0; grp < sub_size; ++grp) {
    OMX_INT set;
    const OMX_F32 *tw = twiddle + grp * sub_num;

    for (set = 0; set < set_count; ++set) {
      OMX_FC32 t;
      const OMX_F32 *in0 = in + set + grp * sub_num;
      const OMX_F32 *in1 = in0 + set_count;
      OMX_F32 *out1 = out0 + (n >> 1);

      // CMUL t, tw, in1
      t.Re = tw[0] * in1[0] - tw[n << 1] * in1[n];
      t.Im = tw[0] * in1[n] + tw[n << 1] * in1[0];

      // CADD out0, in0, t
      out0[0] = in0[0] + t.Re;
      out0[n] = in0[n] + t.Im;

      // CSUB out1, in0, t
      out1[0] = in0[0] - t.Re;
      out1[n] = in0[n] - t.Im;

      out0 += 1;
    }
  }
}
