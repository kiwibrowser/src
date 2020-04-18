/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 * Copyright 2009, Google Inc.
 */

#include "native_client/src/trusted/validator_arm/address_set.h"
#include <stdio.h>
#include <string.h>

namespace nacl_arm_val {

AddressSet::AddressSet(uint32_t base, uint32_t size)
    : base_(base), size_(size), bits_(new uint32_t[(size + 3) / 4]) {
  memset(bits_, 0, sizeof(uint32_t) * ((size + 3) / 4));
}

AddressSet::~AddressSet() {
  delete[] bits_;
}

void AddressSet::add(uint32_t address) {
  if ((address - base_) < size_) {
    uint32_t word_address = (address - base_) / sizeof(uint32_t);

    bits_[word_address / 32] |= 1 << (word_address % 32);
  }
}

bool AddressSet::contains(uint32_t address) const {
  if ((address - base_) < size_) {
    uint32_t word_address = (address - base_) / sizeof(uint32_t);

    return (bits_[word_address / 32] & (1 << (word_address % 32))) != 0;
  } else {
    return false;
  }
}

AddressSet::Iterator AddressSet::begin() const {
  return Iterator(*this, 0, 0);
}

AddressSet::Iterator AddressSet::end() const {
  return Iterator(*this, (size_ + 3) / 4, 0);
}

AddressSet::Iterator::Iterator(const AddressSet &parent,
                               uint32_t index,
                               uint32_t shift)
    : parent_(parent), index_(index), shift_(shift) {
  advance();
}

AddressSet::Iterator &AddressSet::Iterator::operator++() {
  shift_++;   // Skip the current bit, if any, and
  advance();  // seek to the next 1 bit.
  return *this;
}

bool AddressSet::Iterator::operator!=(const AddressSet::Iterator &other) const {
  return index_ != other.index_ || shift_ != other.shift_;
}

uint32_t AddressSet::Iterator::operator*() const {
  return parent_.base_ + 4 * ((index_ * 32) + shift_);
}

void AddressSet::Iterator::advance() {
  uint32_t max_index = (parent_.size_ + 3) / 4;

  for (; index_ < max_index; index_++) {
    uint32_t word = (shift_ > 31)? 0 : parent_.bits_[index_] >> shift_;
    while (word) {
      if (word & 1) return;

      // A meager optimization for sparse words
      if (!(word & 0xFFFF)) {
        word >>= 16;
        shift_ += 16;
      } else if (!(word & 0xFF)) {
        word >>= 8;
        shift_ += 8;
      } else {
        word >>= 1;
        shift_++;
      }
    }
    shift_ = 0;
  }
}

}  // namespace nacl_arm_val
