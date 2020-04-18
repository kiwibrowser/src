/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator_mips/address_set.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

namespace nacl_mips_val {

#if !defined(NDEBUG)
static const int kInstrAlignment = 4;
#endif
static const int kInstrSize = 4;
static const int kWordSize = 32;
static const int kInstrPerWord = 4 * kWordSize;

#define UP_ALLIGN(x) (kInstrSize * ((x + kInstrSize - 1) / kInstrSize))

AddressSet::AddressSet(uint32_t base, uint32_t size)
    : base_(base), size_(size), end_(base + UP_ALLIGN(size) - kInstrSize),
      bits_(new uint32_t[(end_ - base_) / kInstrPerWord + 1]) {
  assert(((base % kInstrAlignment) == 0) && (size > 0));
  memset(bits_, 0, sizeof(uint32_t) * ((end_ - base_) / kInstrPerWord + 1));
}

AddressSet::~AddressSet() {
  delete[] bits_;
}

void AddressSet::Add(uint32_t address) {
  if ((address - base_) < size_) {
    uint32_t word_address = (address - base_) / sizeof(uint32_t);
    bits_[word_address / kWordSize] |= 1 << (word_address % kWordSize);
  }
}

bool AddressSet::Contains(uint32_t address) const {
  if ((address - base_) < size_) {
    uint32_t word_address = (address - base_) / sizeof(uint32_t);

    return bits_[word_address / kWordSize] & (1 << (word_address % kWordSize));
  } else {
    return false;
  }
}

AddressSet::Iterator AddressSet::Begin() const {
  return Iterator(*this, 0, 0);
}

AddressSet::Iterator AddressSet::End() const {
  uint32_t end_index = (end_ - base_) / kInstrPerWord;
  uint32_t end_shift = ((end_ - base_) / kInstrSize) % kWordSize;
  return Iterator(*this, end_index, end_shift);
}

AddressSet::Iterator::Iterator(const AddressSet &parent,
                               uint32_t index,
                               uint32_t shift)
    : parent_(parent), index_(index), shift_(shift) {
  Advance();
}

AddressSet::Iterator &AddressSet::Iterator::Next() {
  shift_++;   // Skip the current bit, if any, and
  Advance();  // seek to the next 1 bit.
  return *this;
}

bool AddressSet::Iterator::Equals(const AddressSet::Iterator &other) const {
  return index_ == other.index_ && shift_ == other.shift_;
}

uint32_t AddressSet::Iterator::GetAddress() const {
  return parent_.base_ + kInstrSize * ((index_ * kWordSize) + shift_);
}

void AddressSet::Iterator::Advance() {
  uint32_t max_index = (parent_.size_ / kInstrPerWord) + 1;

  for (; index_ < max_index; index_++) {
    uint32_t word = (shift_ > 31) ? 0 : parent_.bits_[index_] >> shift_;
    while (word) {
      if (word & 1) return;

      // A meager optimization for sparse words. Check if the first 16 slots are
      // empty, and if they are, skip them. Repeat the same check for 8 slots.
      // Otherwise, we will check bit by bit, to reach the next valid one.
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

  // If we have moved too far, we should limit the address to
  // the last valid address.
  if (index_ == max_index) {
    index_ = (parent_.end_ - parent_.base_) / kInstrPerWord;
    shift_ = ((parent_.end_ - parent_.base_) / kInstrSize) % kWordSize;
  }
}

}  // namespace nacl_mips_val
