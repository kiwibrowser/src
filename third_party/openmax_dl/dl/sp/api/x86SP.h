/*
 *  Copyright (c) 2007-2008 ARM Limited. All Rights Reserved.
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 *  It has been relicensed with permission from the copyright holders.
 */

#ifndef _x86SP_H_
#define _x86SP_H_

#include "dl/api/omxtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

extern OMX_F32 armSP_FFT_F32TwiddleTable[];

typedef struct X86FFTSpec_R_FC32_Tag
{
    OMX_U32 N;
    OMX_F32* pTwiddle;
    // Ping Pong buffer for doing the N/2 point complex FFT.
    OMX_F32* pBuf1;
    OMX_F32* pBuf2;

} X86FFTSpec_R_FC32;

#ifdef __cplusplus
}
#endif

#endif
