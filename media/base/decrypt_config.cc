// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/decrypt_config.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "media/media_buildflags.h"

namespace media {

namespace {

const char* EncryptionModeAsString(EncryptionMode mode) {
  switch (mode) {
    case EncryptionMode::kUnencrypted:
      return "Unencrypted";
    case EncryptionMode::kCenc:
      return "CENC";
    case EncryptionMode::kCbcs:
      return "CBCS";
    default:
      return "Unknown";
  }
}

}  // namespace

// static
std::unique_ptr<DecryptConfig> DecryptConfig::CreateCencConfig(
    const std::string& key_id,
    const std::string& iv,
    const std::vector<SubsampleEntry>& subsamples) {
  return std::make_unique<DecryptConfig>(EncryptionMode::kCenc, key_id, iv,
                                         subsamples, base::nullopt);
}

// static
std::unique_ptr<DecryptConfig> DecryptConfig::CreateCbcsConfig(
    const std::string& key_id,
    const std::string& iv,
    const std::vector<SubsampleEntry>& subsamples,
    base::Optional<EncryptionPattern> encryption_pattern) {
  return std::make_unique<DecryptConfig>(EncryptionMode::kCbcs, key_id, iv,
                                         subsamples,
                                         std::move(encryption_pattern));
}

DecryptConfig::DecryptConfig(
    const EncryptionMode& encryption_mode,
    const std::string& key_id,
    const std::string& iv,
    const std::vector<SubsampleEntry>& subsamples,
    base::Optional<EncryptionPattern> encryption_pattern)
    : encryption_mode_(encryption_mode),
      key_id_(key_id),
      iv_(iv),
      subsamples_(subsamples),
      encryption_pattern_(std::move(encryption_pattern)) {
  // Unencrypted blocks should not have a DecryptConfig.
  DCHECK_NE(encryption_mode_, EncryptionMode::kUnencrypted);
  CHECK_GT(key_id_.size(), 0u);
  CHECK_EQ(iv_.size(), static_cast<size_t>(DecryptConfig::kDecryptionKeySize));

  // Pattern not allowed for non-'cbcs' modes.
  DCHECK(encryption_mode_ == EncryptionMode::kCbcs || !encryption_pattern_);
}

DecryptConfig::~DecryptConfig() = default;

bool DecryptConfig::HasPattern() const {
  return encryption_pattern_.has_value();
}

bool DecryptConfig::Matches(const DecryptConfig& config) const {
  if (key_id() != config.key_id() || iv() != config.iv() ||
      subsamples().size() != config.subsamples().size() ||
      encryption_mode_ != config.encryption_mode_ ||
      encryption_pattern_ != config.encryption_pattern_) {
    return false;
  }

  for (size_t i = 0; i < subsamples().size(); ++i) {
    if ((subsamples()[i].clear_bytes != config.subsamples()[i].clear_bytes) ||
        (subsamples()[i].cypher_bytes != config.subsamples()[i].cypher_bytes)) {
      return false;
    }
  }

  return true;
}

std::ostream& DecryptConfig::Print(std::ostream& os) const {
  os << "key_id:'" << base::HexEncode(key_id_.data(), key_id_.size()) << "'"
     << " iv:'" << base::HexEncode(iv_.data(), iv_.size()) << "'"
     << " mode:" << EncryptionModeAsString(encryption_mode_);

  if (encryption_pattern_) {
    os << " pattern:" << encryption_pattern_->crypt_byte_block() << ":"
       << encryption_pattern_->skip_byte_block();
  }

  os << " subsamples:[";
  for (const SubsampleEntry& entry : subsamples_) {
    os << "(clear:" << entry.clear_bytes << ", cypher:" << entry.cypher_bytes
       << ")";
  }
  os << "]";
  return os;
}

}  // namespace media
