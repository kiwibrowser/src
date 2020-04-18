// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/os_crypt/os_crypt.h"

#include <CommonCrypto/CommonCryptor.h>  // for kCCBlockSizeAES128
#include <stddef.h>

#include "base/command_line.h"
#include "base/debug/leak_annotations.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "components/os_crypt/keychain_password_mac.h"
#include "components/os_crypt/os_crypt_switches.h"
#include "crypto/apple_keychain.h"
#include "crypto/encryptor.h"
#include "crypto/mock_apple_keychain.h"
#include "crypto/symmetric_key.h"

using crypto::AppleKeychain;

namespace {

// Salt for Symmetric key derivation.
const char kSalt[] = "saltysalt";

// Key size required for 128 bit AES.
const size_t kDerivedKeySizeInBits = 128;

// Constant for Symmetic key derivation.
const size_t kEncryptionIterations = 1003;

// TODO(dhollowa): Refactor to allow dependency injection of Keychain.
static bool use_mock_keychain = false;

// Prefix for cypher text returned by current encryption version.  We prefix
// the cypher text with this string so that future data migration can detect
// this and migrate to different encryption without data loss.
const char kEncryptionVersionPrefix[] = "v10";

// This lock is used to make the GetEncrytionKey method thread-safe.
base::LazyInstance<base::Lock>::Leaky g_lock = LAZY_INSTANCE_INITIALIZER;

// Generates a newly allocated SymmetricKey object based on the password found
// in the Keychain.  The generated key is for AES encryption.  Returns NULL key
// in the case password access is denied or key generation error occurs.
crypto::SymmetricKey* GetEncryptionKey() {
  static crypto::SymmetricKey* cached_encryption_key = NULL;
  static bool key_is_cached = false;
  base::AutoLock auto_lock(g_lock.Get());

  if (key_is_cached)
    return cached_encryption_key;

  static bool mock_keychain_command_line_flag =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          os_crypt::switches::kUseMockKeychain);

  std::string password;
  if (use_mock_keychain || mock_keychain_command_line_flag) {
    crypto::MockAppleKeychain keychain;
    password = keychain.GetEncryptionPassword();
  } else {
    AppleKeychain keychain;
    KeychainPassword encryptor_password(keychain);
    password = encryptor_password.GetPassword();
  }

  // Subsequent code must guarantee that the correct key is cached before
  // returning.
  key_is_cached = true;

  if (password.empty())
    return cached_encryption_key;

  std::string salt(kSalt);

  // Create an encryption key from our password and salt. The key is
  // intentionally leaked.
  cached_encryption_key = crypto::SymmetricKey::DeriveKeyFromPassword(
                              crypto::SymmetricKey::AES, password, salt,
                              kEncryptionIterations, kDerivedKeySizeInBits)
                              .release();
  ANNOTATE_LEAKING_OBJECT_PTR(cached_encryption_key);
  DCHECK(cached_encryption_key);
  return cached_encryption_key;
}

}  // namespace

bool OSCrypt::EncryptString16(const base::string16& plaintext,
                              std::string* ciphertext) {
  return EncryptString(base::UTF16ToUTF8(plaintext), ciphertext);
}

bool OSCrypt::DecryptString16(const std::string& ciphertext,
                              base::string16* plaintext) {
  std::string utf8;
  if (!DecryptString(ciphertext, &utf8))
    return false;

  *plaintext = base::UTF8ToUTF16(utf8);
  return true;
}

bool OSCrypt::EncryptString(const std::string& plaintext,
                            std::string* ciphertext) {
  if (plaintext.empty()) {
    *ciphertext = std::string();
    return true;
  }

  crypto::SymmetricKey* encryption_key = GetEncryptionKey();
  if (!encryption_key)
    return false;

  std::string iv(kCCBlockSizeAES128, ' ');
  crypto::Encryptor encryptor;
  if (!encryptor.Init(encryption_key, crypto::Encryptor::CBC, iv))
    return false;

  if (!encryptor.Encrypt(plaintext, ciphertext))
    return false;

  // Prefix the cypher text with version information.
  ciphertext->insert(0, kEncryptionVersionPrefix);
  return true;
}

bool OSCrypt::DecryptString(const std::string& ciphertext,
                            std::string* plaintext) {
  if (ciphertext.empty()) {
    *plaintext = std::string();
    return true;
  }

  // Check that the incoming cyphertext was indeed encrypted with the expected
  // version.  If the prefix is not found then we'll assume we're dealing with
  // old data saved as clear text and we'll return it directly.
  // Credit card numbers are current legacy data, so false match with prefix
  // won't happen.
  if (ciphertext.find(kEncryptionVersionPrefix) != 0) {
    *plaintext = ciphertext;
    return true;
  }

  // Strip off the versioning prefix before decrypting.
  std::string raw_ciphertext =
      ciphertext.substr(strlen(kEncryptionVersionPrefix));

  crypto::SymmetricKey* encryption_key = GetEncryptionKey();
  if (!encryption_key) {
    VLOG(1) << "Decryption failed: could not get the key";
    return false;
  }

  std::string iv(kCCBlockSizeAES128, ' ');
  crypto::Encryptor encryptor;
  if (!encryptor.Init(encryption_key, crypto::Encryptor::CBC, iv))
    return false;

  if (!encryptor.Decrypt(raw_ciphertext, plaintext)) {
    VLOG(1) << "Decryption failed";
    return false;
  }

  return true;
}

void OSCrypt::UseMockKeychain(bool use_mock) {
  use_mock_keychain = use_mock;
}

