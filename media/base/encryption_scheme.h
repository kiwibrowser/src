// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ENCRYPTION_SCHEME_H_
#define MEDIA_BASE_ENCRYPTION_SCHEME_H_

#include <stdint.h>

#include <iosfwd>

#include "media/base/encryption_pattern.h"
#include "media/base/media_export.h"

namespace media {

// Specification of whether and how the stream is encrypted (in whole or part).
class MEDIA_EXPORT EncryptionScheme {
 public:
  // Algorithm and mode used for encryption. CIPHER_MODE_UNENCRYPTED indicates
  // no encryption.
  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.media
  enum CipherMode {
    CIPHER_MODE_UNENCRYPTED,
    CIPHER_MODE_AES_CTR,
    CIPHER_MODE_AES_CBC,
    CIPHER_MODE_MAX = CIPHER_MODE_AES_CBC
  };

  // The default constructor makes an instance that indicates no encryption.
  EncryptionScheme();

  // This constructor allows specification of the cipher mode and the pattern.
  EncryptionScheme(CipherMode mode, const EncryptionPattern& pattern);
  ~EncryptionScheme();

  bool Matches(const EncryptionScheme& other) const;

  bool is_encrypted() const;
  CipherMode mode() const;
  const EncryptionPattern& pattern() const;

 private:
  CipherMode mode_ = CIPHER_MODE_UNENCRYPTED;
  EncryptionPattern pattern_;

  // Allow copy and assignment.
};

// For logging use only.
MEDIA_EXPORT std::ostream& operator<<(
    std::ostream& os,
    const EncryptionScheme& encryption_scheme);

}  // namespace media

#endif  // MEDIA_BASE_ENCRYPTION_SCHEME_H_
