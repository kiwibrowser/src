/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Secure RNG implementation.
 */

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_secure_random.h"

uint32_t NaClSecureRngDefaultGenUint32(struct NaClSecureRngIf *self) {
  uint32_t rv;

  rv  = (*self->vtbl->GenByte)(self);
  rv <<= 8;
  rv |= (*self->vtbl->GenByte)(self);
  rv <<= 8;
  rv |= (*self->vtbl->GenByte)(self);
  rv <<= 8;
  rv |= (*self->vtbl->GenByte)(self);
  return rv;
}

void NaClSecureRngDefaultGenBytes(struct NaClSecureRngIf  *self,
                                  uint8_t                 *buf,
                                  size_t                  nbytes) {
  size_t i;

  for (i = 0; i < nbytes; ++i) {
    buf[i] = (*self->vtbl->GenByte)(self);
  }
}

uint32_t NaClSecureRngDefaultUniform(struct NaClSecureRngIf *self,
                                     uint32_t               range_max) {
  uint32_t bias;
  uint32_t v;

  /*
   * First, is range_max a power of 2?  If so, we can just keep the
   * low order bits.
   */
  if (0 == (range_max & (range_max - 1))) {
    return (*self->vtbl->GenUint32)(self) & (range_max - 1);
  }
  /* post condition: range_max is not a power of 2 */

  /*
   * Generator output range is uniform in [0, ~(uint32_t) 0] or [0, 2^{32}).
   *
   * NB: Number of integer values in the range [0, n) is n,
   * and number of integer values in the range [m, n) is n-m,
   * (n and m are integers).
   */
  bias = ((~(uint32_t) 0) % range_max) + 1;
  /*
   * bias = (2^{32}-1 \bmod range_max) + 1
   *
   * 1 <= bias <= range_max.
   *
   * Aside: bias = range_max is impossible.  Here's why:
   *
   * Suppose bias = range_max.  Then,
   *
   * 2^{32}-1 \bmod range_max + 1 = range_max
   * 2^{32}-1 \bmod range_max = range_max - 1
   * 2^{32}-1 = k range_max + range_max - 1 for some k \in Z.
   * 2^{32}-1 = (k+1) range_max - 1
   * 2^{32} = (k+1) range_max
   *
   * Since range_max evenly divides the right hand side, it must
   * divide the left.  However, the only divisors of 2^{32} are powers
   * of 2.  Thus, range_max must also be a power of 2.  =><=
   *
   * Therefore, bias \ne range_max.  QED.
   *
   * post condition:  1 <= bias < range_max.
   */


  do {
    v = (*self->vtbl->GenUint32)(self);
    /* v is uniform in [0, 2^{32}) */
  } while (v < bias);
  /*
   * v is uniform in [bias, 2^{32}).
   *
   * the number of integers in the half-open interval is
   *
   *   2^{32} - bias = 2^{32} - ((2^{32}-1 \bmod range_max + 1))
   *                 = 2^{32}-1 - (2^{32}-1 \bmod range_max)
   *                 = range_max * \lfloor (2^{32}-1)/range_max \rfloor
   *
   * and range_max clearly divides the size of the range.
   *
   * The mapping v \rightarrow v % range_max takes values from [bias,
   * 2^{32}) to [0, range_max).  Each integer in the range interval
   * [0, range_max) will have exactly \lfloor (2^{32}-1)/range_max
   * \rfloor preimages from the domain interval.
   *
   * Therefore, v % range_max is uniform over [0, range_max).  QED.
   */

  return v % range_max;
}
