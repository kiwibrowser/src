// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_DECRYPT_CONFIG_H_
#define MEDIA_BASE_DECRYPT_CONFIG_H_

#include <stdint.h>

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/optional.h"
#include "media/base/encryption_pattern.h"
#include "media/base/media_export.h"
#include "media/base/subsample_entry.h"

namespace media {

// The encryption mode. The definitions are from ISO/IEC 23001-7:2016.
// TODO(crbug.com/825041): Merge this with existing media::EncryptionScheme.
enum class EncryptionMode {
  kUnencrypted = 0,
  kCenc,  // 'cenc' subsample encryption using AES-CTR mode.
  kCbcs,  // 'cbcs' pattern encryption using AES-CBC mode.
  kMaxValue = kCbcs
};

// Contains all information that a decryptor needs to decrypt a media sample.
class MEDIA_EXPORT DecryptConfig {
 public:
  // Keys are always 128 bits.
  static const int kDecryptionKeySize = 16;

  // |key_id| is the ID that references the decryption key for this sample.
  // |iv| is the initialization vector defined by the encrypted format.
  //   Currently |iv| must be 16 bytes as defined by WebM and ISO. It must
  //   be provided.
  // |subsamples| defines the clear and encrypted portions of the sample as
  //   described above. A decrypted buffer will be equal in size to the sum
  //   of the subsample sizes.
  // |encryption_pattern| is the pattern used ('cbcs' only). It is optional
  //   as Common encryption of MPEG-2 transport streams v1 (23009-1:2014)
  //   does not specify patterns for cbcs encryption mode. The pattern is
  //   assumed to be 1:9 for video and 1:0 for audio.
  static std::unique_ptr<DecryptConfig> CreateCencConfig(
      const std::string& key_id,
      const std::string& iv,
      const std::vector<SubsampleEntry>& subsamples);
  static std::unique_ptr<DecryptConfig> CreateCbcsConfig(
      const std::string& key_id,
      const std::string& iv,
      const std::vector<SubsampleEntry>& subsamples,
      base::Optional<EncryptionPattern> encryption_pattern);

  DecryptConfig(const EncryptionMode& encryption_mode,
                const std::string& key_id,
                const std::string& iv,
                const std::vector<SubsampleEntry>& subsamples,
                base::Optional<EncryptionPattern> encryption_pattern);
  ~DecryptConfig();

  const std::string& key_id() const { return key_id_; }
  const std::string& iv() const { return iv_; }
  const std::vector<SubsampleEntry>& subsamples() const { return subsamples_; }
  const EncryptionMode& encryption_mode() const { return encryption_mode_; }
  const base::Optional<EncryptionPattern>& encryption_pattern() const {
    return encryption_pattern_;
  };

  // Returns whether this config has EncryptionPattern set or not.
  bool HasPattern() const;

  // Returns true if all fields in |config| match this config.
  bool Matches(const DecryptConfig& config) const;

  // Prints to std::ostream.
  std::ostream& Print(std::ostream& os) const;

 private:
  const EncryptionMode encryption_mode_;
  const std::string key_id_;

  // Initialization vector.
  const std::string iv_;

  // Subsample information. May be empty for some formats, meaning entire frame
  // (less data ignored by data_offset_) is encrypted.
  const std::vector<SubsampleEntry> subsamples_;

  // Only specified if |encryption_mode_| requires a pattern.
  base::Optional<EncryptionPattern> encryption_pattern_;

  DISALLOW_COPY_AND_ASSIGN(DecryptConfig);
};

}  // namespace media

inline std::ostream& operator<<(std::ostream& os,
                                const media::DecryptConfig& obj) {
  return obj.Print(os);
}

#endif  // MEDIA_BASE_DECRYPT_CONFIG_H_
