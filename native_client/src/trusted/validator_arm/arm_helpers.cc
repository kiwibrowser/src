/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>

#include "native_client/src/trusted/validator_arm/arm_helpers.h"

namespace nacl_arm_dec {

uint64_t VFPExpandImm(uint32_t imm8, int n) {
  uint64_t a = (imm8 & 0x80) >> 7;
  uint64_t b = (imm8 & 0x40) >> 6;
  uint64_t bbbbb = b ? 0x1F : 0x0;
  uint64_t bbbbbbbb = b ? 0xFF : 0x0;
  uint64_t B = ~b & 0x1;
  uint64_t cdefgh = imm8 & 0x2F;
  switch (n) {
  default:
    assert(0);
    return 0;
  case 32:
    return ((a << 12) | (B << 11) | (bbbbb << 6) | cdefgh) << 19;
  case 64:
    return ((a << 15) | (B << 14) | (bbbbbbbb << 6) | cdefgh) << 48;
  }
}

}  // namespace nacl_arm_dec
