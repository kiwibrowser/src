// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BASE_FAKE_ENCRYPTOR_H_
#define COMPONENTS_SYNC_BASE_FAKE_ENCRYPTOR_H_

#include <string>

#include "base/compiler_specific.h"
#include "components/sync/base/encryptor.h"

namespace syncer {

// Encryptor which simply base64-encodes the plaintext to get the
// ciphertext.  Obviously, this should be used only for testing.
class FakeEncryptor : public Encryptor {
 public:
  ~FakeEncryptor() override;

  bool EncryptString(const std::string& plaintext,
                     std::string* ciphertext) override;

  bool DecryptString(const std::string& ciphertext,
                     std::string* plaintext) override;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_BASE_FAKE_ENCRYPTOR_H_
