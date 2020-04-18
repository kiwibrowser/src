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
#include <stdbool.h>

extern void x86SP_FFT_CToC_FC32_Fwd_Radix2_fs(
    const OMX_F32 *in,
    OMX_F32 *out,
    OMX_INT n);

extern void x86SP_FFT_CToC_FC32_Inv_Radix2_fs(
    const OMX_F32 *in,
    OMX_F32 *out,
    OMX_INT n);

extern void x86SP_FFT_CToC_FC32_Fwd_Radix2_ms(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n,
    OMX_INT sub_size,
    OMX_INT sub_num);

extern void x86SP_FFT_CToC_FC32_Inv_Radix2_ms(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n,
    OMX_INT sub_size,
    OMX_INT sub_num);

extern void x86SP_FFT_CToC_FC32_Fwd_Radix2_ls(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n);

extern void x86SP_FFT_CToC_FC32_Inv_Radix2_ls(
    const OMX_F32 *in,
    OMX_F32 *out,
    const OMX_F32 *twiddle,
    OMX_INT n);

OMX_F32* x86SP_F32_radix2_kernel_OutOfPlace(
    const OMX_F32 *src,
    // Two Ping Pong buffers for out of place kernel.
    OMX_F32 *buf1,
    OMX_F32 *buf2,
    const OMX_F32 *twiddle,
    OMX_INT n,
    bool forward_fft) {
  OMX_INT sub_size;
  OMX_INT sub_num;
  OMX_INT n_by_2 = n >> 1;
  OMX_F32 *in = buf1;
  OMX_F32 *out = buf2;

  if (forward_fft)
    x86SP_FFT_CToC_FC32_Fwd_Radix2_fs(src, in, n);
  else
    x86SP_FFT_CToC_FC32_Inv_Radix2_fs(src, in, n);

  for (sub_size = 2, sub_num = n_by_2;
       sub_size < n_by_2;
       sub_size = sub_size << 1, sub_num = sub_num >> 1) {

    if (forward_fft) {
      x86SP_FFT_CToC_FC32_Fwd_Radix2_ms(in, out, twiddle,
                                        n, sub_size, sub_num);
    } else {
      x86SP_FFT_CToC_FC32_Inv_Radix2_ms(in, out, twiddle,
                                        n, sub_size, sub_num);
    }

    OMX_F32 *temp = out;
    out = in;
    in = temp;
  }

  // If sub_num <= 1, no need to do the last stage.
  if (sub_num <= 1)
    return in;

  if (forward_fft)
    x86SP_FFT_CToC_FC32_Fwd_Radix2_ls(in, out, twiddle, n);
  else
    x86SP_FFT_CToC_FC32_Inv_Radix2_ls(in, out, twiddle, n);

  return out;
}
