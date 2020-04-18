// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/encryption_scheme.h"

#include <ostream>

#include "base/logging.h"

namespace media {

EncryptionScheme::EncryptionScheme() = default;

EncryptionScheme::EncryptionScheme(CipherMode mode,
                                   const EncryptionPattern& pattern)
    : mode_(mode), pattern_(pattern) {}

EncryptionScheme::~EncryptionScheme() = default;

bool EncryptionScheme::is_encrypted() const {
  return mode_ != CIPHER_MODE_UNENCRYPTED;
}

EncryptionScheme::CipherMode EncryptionScheme::mode() const {
  return mode_;
}

const EncryptionPattern& EncryptionScheme::pattern() const {
  return pattern_;
}

bool EncryptionScheme::Matches(const EncryptionScheme& other) const {
  return mode_ == other.mode_ && pattern_ == other.pattern_;
}

std::ostream& operator<<(std::ostream& os,
                         const EncryptionScheme& encryption_scheme) {
  if (!encryption_scheme.is_encrypted())
    return os << "Unencrypted";

  bool pattern_in_effect = encryption_scheme.pattern().IsInEffect();

  if (encryption_scheme.mode() == EncryptionScheme::CIPHER_MODE_AES_CTR &&
      !pattern_in_effect) {
    return os << "CENC";
  }

  if (encryption_scheme.mode() == EncryptionScheme::CIPHER_MODE_AES_CBC &&
      pattern_in_effect) {
    return os << "CBCS";
  }

  NOTREACHED();
  return os << "Unknown (mode = " << encryption_scheme.mode()
            << ", pattern_in_effect = " << pattern_in_effect << ")";
}

}  // namespace media
