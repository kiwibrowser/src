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

#include <stdint.h>

#include "dl/api/omxtypes.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/api/mipsSP.h"

static OMX_U16 SplitRadixPermutation(int i, int size, int inverse) {
  int m;
  if (size <= 2)
    return (i & 1);
  m = size >> 1;
  if (!(i & m))
    return SplitRadixPermutation(i, m, inverse) * 2;
  m >>= 1;
  if (inverse == !(i & m))
    return SplitRadixPermutation(i, m, inverse) * 4 + 1;

  return SplitRadixPermutation(i, m, inverse) * 4 - 1;
}

static void InitFFTOffsetsLUT(OMX_U16* offset_table,
                              int offset,
                              int size,
                              OMX_U32* index) {
  if (size < 16) {
    offset_table[*index] = (OMX_U16)(offset >> 2);
    (*index)++;
  } else {
    InitFFTOffsetsLUT(offset_table, offset, size >> 1, index);
    InitFFTOffsetsLUT(offset_table, offset + (size >> 1), size >> 2, index);
    InitFFTOffsetsLUT(offset_table, offset + 3 * (size >> 2), size >> 2, index);
  }
}

OMXResult omxSP_FFTInit_R_F32(OMXFFTSpec_R_F32* pFFTSpec, OMX_INT order) {
  OMX_U32 n;
  uint32_t fft_size;
  OMX_U16* p_bit_rev;
  OMX_U16* p_bit_rev_inv;
  OMX_U16* p_offset;
  OMX_F32* p_twiddle;
  OMX_F32* p_buf;
  OMX_U32 tmp;
  MIPSFFTSpec_R_FC32* pFFTStruct = (MIPSFFTSpec_R_FC32*)pFFTSpec;

  if (!pFFTSpec || (order < 1) || (order > TWIDDLE_TABLE_ORDER))
    return OMX_Sts_BadArgErr;

  /* For order larger than 4, compute Real FFT as Complex FFT of (order - 1). */
  if (order > 4)
    fft_size = 1 << (order - 1);
  else
    fft_size = 1 << order;

  p_bit_rev = (OMX_U16*)((OMX_S8*)pFFTSpec + sizeof(MIPSFFTSpec_R_FC32));
  /* Align to 32 byte boundary. */
  tmp = ((uintptr_t)p_bit_rev) & 31;
  if (tmp)
    p_bit_rev = (OMX_U16*)((OMX_S8*)p_bit_rev + (32 - tmp));

  p_bit_rev_inv = (OMX_U16*)((OMX_S8*)p_bit_rev + fft_size * sizeof(OMX_U16));
  /* Align to 32 byte boundary. */
  tmp = ((uintptr_t)p_bit_rev_inv) & 31;
  if (tmp)
    p_bit_rev_inv = (OMX_U16*)((OMX_S8*)p_bit_rev_inv + (32 - tmp));

  p_offset = (OMX_U16*)((OMX_S8*)p_bit_rev_inv + fft_size * sizeof(OMX_U16));
  /* Align to 32 byte boundary. */
  tmp = ((uintptr_t)p_offset) & 31;
  if (tmp)
    p_offset = (OMX_U16*)((OMX_S8*)p_offset + (32 - tmp));

  if (order < 5) {
    p_twiddle = (OMX_F32*)((OMX_S8*)p_offset +
                           ((SUBTRANSFORM_CONST >> (16 - order)) | 1) *
                               sizeof(OMX_U16));
  } else {
    p_twiddle = (OMX_F32*)((OMX_S8*)p_offset +
                           ((SUBTRANSFORM_CONST >> (17 - order)) | 1) *
                               sizeof(OMX_U16));
  }

  /* Align to 32 byte boundary. */
  tmp = ((uintptr_t)p_twiddle) & 31;
  if (tmp)
    p_twiddle = (OMX_F32*)((OMX_S8*)p_twiddle + (32 - tmp));

  if (order > 3)
    p_buf =
        (OMX_F32*)((OMX_S8*)p_twiddle + (1 << (order - 2)) * sizeof(OMX_F32));
  else
    p_buf = (OMX_F32*)((OMX_S8*)p_twiddle + sizeof(OMX_F32));

  /* Align to 32 byte boundary. */
  tmp = ((uintptr_t)p_buf) & 31;
  if (tmp)
    p_buf = (OMX_F32*)((OMX_S8*)p_buf + (32 - tmp));

  /* Calculate BitRevInv indexes. */
  for (uint32_t i = 0; i < fft_size; ++i)
    p_bit_rev_inv[-SplitRadixPermutation(i, fft_size, 0) & (fft_size - 1)] = i;

  /* Calculate BitRev indexes. */
  for (uint32_t i = 0; i < fft_size; ++i)
    p_bit_rev[p_bit_rev_inv[i]] = i;

  /* Calculate Offsets. */
  n = 0;
  if (order < 5)
    InitFFTOffsetsLUT(p_offset, 0, 1 << order, &n);
  else
    InitFFTOffsetsLUT(p_offset, 0, 1 << (order - 1), &n);

  /* Fill the twiddle table */
  if (order > 3) {
    tmp = 1 << (TWIDDLE_TABLE_ORDER - order);
    for (n = 0; n < (1u << (order - 2)); ++n) {
      p_twiddle[n] = mipsSP_FFT_F32TwiddleTable[n * tmp];
    }
  }

  /*
   * Double-check if the offset tables are initialized correctly.
   * Note: the bit-reverse tables and the initialization algorithm for
   * pFFTStruct->pOffset table are thoroughly tested, so this check is
   * probabaly redundant. However, keeping this just to make sure the offsets
   * will not exceed the buffer boundaries.
   */
  if (order > 1) {
    /*
     * In the case of order = 1 there is no table accesses, so there is no need
     * for any boundary checks.
     */
    if (order == 2) {
      /* Only check the offsets for the p_bit_rev_inv table. */
      for (uint32_t i = 0; i < fft_size; ++i) {
        if (p_bit_rev_inv[i] >= fft_size)
          return OMX_Sts_BadArgErr;
      }
    } else if (order < 5) {
      /* Check for p_offset table. */
      uint32_t shift = 2;
      uint32_t over = 4;
      uint32_t num_transforms = (SUBTRANSFORM_CONST >> (16 - order)) | 1;
      for (int i = 2; i < order; ++i) {
        for (uint32_t j = 0; j < num_transforms; ++j) {
          if (((p_offset[j] << shift) + over - 1) >= fft_size)
            return OMX_Sts_BadArgErr;
        }
        shift++;
        over <<= 1;
        num_transforms = (num_transforms >> 1) | 1;
      }
      /* Check for bit-reverse tables. */
      for (uint32_t i = 0; i < fft_size; ++i) {
        if ((p_bit_rev[i] >= fft_size) || (p_bit_rev_inv[i] >= fft_size))
          return OMX_Sts_BadArgErr;
      }
    } else {
      /* Check for p_offset table. */
      uint32_t shift = 2;
      uint32_t over = 4;
      uint32_t num_transforms = (SUBTRANSFORM_CONST >> (17 - order)) | 1;
      for (int i = 2; i < order; ++i) {
        for (uint32_t j = 0; j < num_transforms; ++j) {
          if (((p_offset[j] << shift) + over - 1) >= fft_size)
            return OMX_Sts_BadArgErr;
        }
        shift++;
        over <<= 1;
        num_transforms = (num_transforms >> 1) | 1;
      }
      /* Check for bit-reverse tables. */
      for (uint32_t i = 0; i < fft_size; ++i) {
        if ((p_bit_rev[i] >= fft_size) || (p_bit_rev_inv[i] >= fft_size))
          return OMX_Sts_BadArgErr;
      }
    }
  }

  pFFTStruct->order = order;
  pFFTStruct->pBitRev = p_bit_rev;
  pFFTStruct->pBitRevInv = p_bit_rev_inv;
  pFFTStruct->pOffset = (const OMX_U16*)p_offset;
  pFFTStruct->pTwiddle = (const OMX_F32*)p_twiddle;
  pFFTStruct->pBuf = p_buf;

  return OMX_Sts_NoErr;
}
