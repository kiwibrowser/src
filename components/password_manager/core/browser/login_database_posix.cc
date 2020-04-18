// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/login_database.h"

#include "base/strings/utf_string_conversions.h"
#include "components/os_crypt/os_crypt.h"

namespace password_manager {

LoginDatabase::EncryptionResult LoginDatabase::EncryptedString(
    const base::string16& plain_text,
    std::string* cipher_text) const {
  if (!use_encryption_) {
    *cipher_text = base::UTF16ToUTF8(plain_text);
    return ENCRYPTION_RESULT_SUCCESS;
  }

  return OSCrypt::EncryptString16(plain_text, cipher_text)
             ? ENCRYPTION_RESULT_SUCCESS
             : ENCRYPTION_RESULT_SERVICE_FAILURE;
}

LoginDatabase::EncryptionResult LoginDatabase::DecryptedString(
    const std::string& cipher_text,
    base::string16* plain_text) const {
  if (!use_encryption_) {
    *plain_text = base::UTF8ToUTF16(cipher_text);
    return ENCRYPTION_RESULT_SUCCESS;
  }

  return OSCrypt::DecryptString16(cipher_text, plain_text)
             ? ENCRYPTION_RESULT_SUCCESS
             : ENCRYPTION_RESULT_SERVICE_FAILURE;
}

}  // namespace password_manager
