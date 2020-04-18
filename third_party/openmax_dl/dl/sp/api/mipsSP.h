/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#ifndef _MIPSSP_H_
#define _MIPSSP_H_

#include "dl/api/omxtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Number of sub-transforms performed in the final stage of the 65536-size FFT.
 * The number of sub-transforms for all other stages can be derived from this
 * number. This sequence of numbers is equivalent to the Jacobsthal number
 * sequence (see http://en.wikipedia.org/wiki/Jacobsthal_number).
 */
#define SUBTRANSFORM_CONST (0x2aab)
#define SQRT1_2 (0.7071067812f) /* sqrt(0.5f) */

extern OMX_F32 mipsSP_FFT_F32TwiddleTable[];

typedef struct MIPSFFTSpec_R_FC32_Tag {
  OMX_U32 order;
  OMX_U16* pBitRev;
  OMX_U16* pBitRevInv;
  const OMX_U16* pOffset;
  const OMX_F32* pTwiddle;
  OMX_F32* pBuf;
} MIPSFFTSpec_R_FC32;

#ifdef __cplusplus
}
#endif

#endif
