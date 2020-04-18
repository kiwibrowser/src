/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 *  This file was originally licensed as follows. It has been
 *  relicensed with permission from the copyright holders.
 */

/**
 *
 * File Name:  armSP.h
 * OpenMAX DL: v1.0.2
 * Last Modified Revision:   7014
 * Last Modified Date:       Wed, 01 Aug 2007
 *
 * (c) Copyright 2007-2008 ARM Limited. All Rights Reserved.
 *
 *
 *
 * File: armSP.h
 * Brief: Declares API's/Basic Data types used across the OpenMAX Signal Processing domain
 *
 */
#ifndef _armSP_H_
#define _armSP_H_

#include <stdint.h>

#include "dl/api/omxtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/** FFT Specific declarations */
extern  OMX_S32 armSP_FFT_S32TwiddleTable[1026];
extern OMX_F32 armSP_FFT_F32TwiddleTable[];

typedef struct  ARMsFFTSpec_SC32_Tag
{
    OMX_U32     N;
    OMX_U16     *pBitRev;
    OMX_SC32    *pTwiddle;
    OMX_SC32    *pBuf;
}ARMsFFTSpec_SC32;


typedef struct  ARMsFFTSpec_SC16_Tag
{
    OMX_U32     N;
    OMX_U16     *pBitRev;
    OMX_SC16    *pTwiddle;
    OMX_SC16    *pBuf;
}ARMsFFTSpec_SC16;

typedef struct  ARMsFFTSpec_R_SC32_Tag
{
    OMX_U32     N;
    OMX_U16     *pBitRev;
    OMX_SC32    *pTwiddle;
    OMX_S32     *pBuf;
}ARMsFFTSpec_R_SC32;

typedef struct  ARMsFFTSpec_R_SC16_Tag
{
    OMX_U32     N;
    OMX_U16     *pBitRev;
    OMX_SC16    *pTwiddle;
    OMX_S16     *pBuf;
} ARMsFFTSpec_R_SC16;

typedef struct ARMsFFTSpec_R_FC32_Tag
{
    OMX_U32 N;
    OMX_U16* pBitRev;
    OMX_FC32* pTwiddle;
    OMX_F32* pBuf;
} ARMsFFTSpec_R_FC32;

typedef struct ARMsFFTSpec_FC32_Tag
{
    OMX_U32 N;
    OMX_U16* pBitRev;
    OMX_FC32* pTwiddle;
    OMX_FC32* pBuf;
} ARMsFFTSpec_FC32;

/*
 * Compute log2(x), where x must be a power of 2.
 */
static inline long fastlog2(long x) {
  long out;
  __asm__ ("clz %0,%1\n\t"
      "sub %0, %0, #63\n\t"
      "neg %0, %0\n\t"
      : "=r"(out)
      : "r"(x)
      :);
  return out;
}

/*
 *  Validate args. All pointers must be non-NULL; the source and
 *  destination pointers must be aligned on a 32-byte boundary; the
 *  FFT spec must have non-NULL pointers; and the FFT size must be
 *  within range.
 */
static inline int validateParametersFC32(const void* pSrc,
					 const void* pDst,
					 const ARMsFFTSpec_FC32* pFFTSpec) {
  return pSrc && pDst && pFFTSpec && !(((uintptr_t)pSrc) & 31) &&
         !(((uintptr_t)pDst) & 31) && pFFTSpec->pTwiddle && pFFTSpec->pBuf &&
         (pFFTSpec->N >= 2) && (pFFTSpec->N <= (1 << TWIDDLE_TABLE_ORDER));
}

static inline int validateParametersF32(const void* pSrc,
					const void* pDst,
					const ARMsFFTSpec_R_FC32* pFFTSpec) {
  return pSrc && pDst && pFFTSpec && !(((uintptr_t)pSrc) & 31) &&
         !(((uintptr_t)pDst) & 31) && pFFTSpec->pTwiddle && pFFTSpec->pBuf &&
         (pFFTSpec->N >= 2) && (pFFTSpec->N <= (1 << TWIDDLE_TABLE_ORDER));
}

#ifdef __cplusplus
}
#endif

#endif

/*End of File*/



