// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_LOGIN_AUTH_CHALLENGE_RESPONSE_KEY_H_
#define CHROMEOS_LOGIN_AUTH_CHALLENGE_RESPONSE_KEY_H_

#include <string>
#include <vector>

#include "chromeos/chromeos_export.h"

namespace chromeos {

// This class contains information about a challenge-response key for user
// authentication. This includes information about the public key of the
// cryptographic key to be challenged, as well as the signature algorithms
// supported for the challenge.
class CHROMEOS_EXPORT ChallengeResponseKey {
 public:
  // Cryptographic signature algorithm type for challenge requests.
  enum class SignatureAlgorithm {
    kRsassaPkcs1V15Sha1,
    kRsassaPkcs1V15Sha256,
    kRsassaPkcs1V15Sha384,
    kRsassaPkcs1V15Sha512,
  };

  ChallengeResponseKey();
  ChallengeResponseKey(const ChallengeResponseKey& other);
  ~ChallengeResponseKey();

  bool operator==(const ChallengeResponseKey& other) const;
  bool operator!=(const ChallengeResponseKey& other) const;

  // Getter and setter for the DER-encoded blob of the X.509 Subject Public Key
  // Info.
  const std::string& public_key_spki_der() const {
    return public_key_spki_der_;
  }
  void set_public_key_spki_der(const std::string& public_key_spki_der) {
    public_key_spki_der_ = public_key_spki_der;
  }

  // Getter and setter for the list of supported signature algorithms, in the
  // order of preference (starting from the most preferred). Absence of this
  // field denotes that the key cannot be used for signing.
  const std::vector<SignatureAlgorithm>& signature_algorithms() const {
    return signature_algorithms_;
  }
  void set_signature_algorithms(
      const std::vector<SignatureAlgorithm>& signature_algorithms) {
    signature_algorithms_ = signature_algorithms;
  }

 private:
  std::string public_key_spki_der_;
  std::vector<SignatureAlgorithm> signature_algorithms_;
};

}  // namespace chromeos

#endif  // CHROMEOS_LOGIN_AUTH_CHALLENGE_RESPONSE_KEY_H_
