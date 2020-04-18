/*
 * Copyright 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 * Copyright 2012, Google Inc.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_MODEL_INL_H
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_MODEL_INL_H
/*
 * Inline definitions for the classes defined in model.h
 */

namespace nacl_mips_dec {

Register::Register(uint32_t number) : _number(number) {}
uint32_t Register::Bitmask() const {
  if (_number == 31) return 0;

  return (1 << _number);
}

bool Register::Equals(const Register &other) const {
  return _number == other._number;
}

RegisterList::RegisterList(uint32_t bits) : _bits(bits) {}
RegisterList::RegisterList(Register reg) : _bits(reg.Bitmask()) {}

bool RegisterList::operator[](Register reg) const {
  return _bits & reg.Bitmask();
}

bool RegisterList::ContainsAll(const RegisterList other) const {
  return (_bits & other._bits) == other._bits;
}

bool RegisterList::ContainsAny(const RegisterList other) const {
  return _bits & other._bits;
}

const RegisterList RegisterList::operator&(const RegisterList other) const {
  return RegisterList(_bits & other._bits);
}

inline Instruction::Instruction(uint32_t bits) : _bits(bits) {}

inline uint32_t Instruction::Bits(int hi, int lo) const {
  uint32_t right_justified = _bits >> lo;
  int bit_count = hi - lo + 1;
  uint32_t mask = (1 << bit_count) - 1;
  return right_justified & mask;
}

inline const Register Instruction::Reg(int hi, int lo) const {
  return Register(Bits(hi, lo));
}

inline bool Instruction::Bit(int index) const {
  return (_bits >> index) & 1;
}

inline uint32_t Instruction::operator&(uint32_t mask) const {
  return _bits & mask;
}

}  // namespace

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_MODEL_INL_H
