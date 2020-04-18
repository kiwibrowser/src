// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/encryption_pattern.h"

namespace media {

EncryptionPattern::EncryptionPattern() = default;

EncryptionPattern::EncryptionPattern(uint32_t crypt_byte_block,
                                     uint32_t skip_byte_block)
    : crypt_byte_block_(crypt_byte_block), skip_byte_block_(skip_byte_block) {}

EncryptionPattern::EncryptionPattern(const EncryptionPattern& rhs) = default;

EncryptionPattern& EncryptionPattern::operator=(const EncryptionPattern& rhs) =
    default;

EncryptionPattern::~EncryptionPattern() = default;

bool EncryptionPattern::IsInEffect() const {
  // ISO/IEC 23001-7(2016), section 10.3, discussing 'cens' pattern encryption
  // scheme, states "Tracks other than video are protected using whole-block
  // full-sample encryption as specified in 9.7 and hence skip_byte_block
  // SHALL be 0." So pattern is in effect as long as |crypt_byte_block_| is set.
  return crypt_byte_block_ != 0;
}

bool EncryptionPattern::operator==(const EncryptionPattern& other) const {
  return crypt_byte_block_ == other.crypt_byte_block_ &&
         skip_byte_block_ == other.skip_byte_block_;
}

bool EncryptionPattern::operator!=(const EncryptionPattern& other) const {
  return !operator==(other);
}

}  // namespace media
