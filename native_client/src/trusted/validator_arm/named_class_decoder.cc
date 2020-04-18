/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator_arm/named_class_decoder.h"

namespace nacl_arm_test {

nacl_arm_dec::SafetyLevel NamedClassDecoder::
safety(nacl_arm_dec::Instruction i) const {
  return wrapped_decoder_.safety(i);
}

nacl_arm_dec::RegisterList NamedClassDecoder::
defs(nacl_arm_dec::Instruction i) const {
  return wrapped_decoder_.defs(i);
}

bool NamedClassDecoder::base_address_register_writeback_small_immediate(
    nacl_arm_dec::Instruction i) const {
  return wrapped_decoder_.base_address_register_writeback_small_immediate(i);
}

nacl_arm_dec::Register NamedClassDecoder::
base_address_register(nacl_arm_dec::Instruction i) const {
  return wrapped_decoder_.base_address_register(i);
}

bool NamedClassDecoder::is_literal_load(nacl_arm_dec::Instruction i) const {
  return wrapped_decoder_.is_literal_load(i);
}

nacl_arm_dec::Register NamedClassDecoder::
branch_target_register(nacl_arm_dec::Instruction i) const {
  return wrapped_decoder_.branch_target_register(i);
}

bool NamedClassDecoder::is_relative_branch(nacl_arm_dec::Instruction i) const {
  return wrapped_decoder_.is_relative_branch(i);
}

int32_t NamedClassDecoder::
branch_target_offset(nacl_arm_dec::Instruction i) const {
  return wrapped_decoder_.branch_target_offset(i);
}

bool NamedClassDecoder::
is_literal_pool_head(nacl_arm_dec::Instruction i) const {
  return wrapped_decoder_.is_literal_pool_head(i);
}

bool NamedClassDecoder::
clears_bits(nacl_arm_dec::Instruction i, uint32_t mask) const {
  return wrapped_decoder_.clears_bits(i, mask);
}

bool NamedClassDecoder::sets_Z_if_bits_clear(nacl_arm_dec::Instruction i,
                                             nacl_arm_dec::Register r,
                                             uint32_t mask) const {
  return wrapped_decoder_.sets_Z_if_bits_clear(i, r, mask);
}

}  // namespace nacl_arm_test
