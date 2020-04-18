// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/nigori.h"

#include <stdint.h>

#include <sstream>
#include <vector>

#include "base/base64.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/sys_byteorder.h"
#include "crypto/encryptor.h"
#include "crypto/hmac.h"
#include "crypto/random.h"
#include "crypto/symmetric_key.h"

using base::Base64Encode;
using base::Base64Decode;
using crypto::HMAC;
using crypto::SymmetricKey;

namespace syncer {

// NigoriStream simplifies the concatenation operation of the Nigori protocol.
class NigoriStream {
 public:
  // Append the big-endian representation of the length of |value| with 32 bits,
  // followed by |value| itself to the stream.
  NigoriStream& operator<<(const std::string& value) {
    uint32_t size = base::HostToNet32(value.size());

    stream_.write(reinterpret_cast<char*>(&size), sizeof(uint32_t));
    stream_ << value;
    return *this;
  }

  // Append the big-endian representation of the length of |type| with 32 bits,
  // followed by the big-endian representation of the value of |type|, with 32
  // bits, to the stream.
  NigoriStream& operator<<(const Nigori::Type type) {
    uint32_t size = base::HostToNet32(sizeof(uint32_t));
    stream_.write(reinterpret_cast<char*>(&size), sizeof(uint32_t));
    uint32_t value = base::HostToNet32(type);
    stream_.write(reinterpret_cast<char*>(&value), sizeof(uint32_t));
    return *this;
  }

  std::string str() { return stream_.str(); }

 private:
  std::ostringstream stream_;
};

// static
const char Nigori::kSaltSalt[] = "saltsalt";

Nigori::Nigori() {}

Nigori::~Nigori() {}

bool Nigori::InitByDerivation(const std::string& hostname,
                              const std::string& username,
                              const std::string& password) {
  NigoriStream salt_password;
  salt_password << username << hostname;

  // Suser = PBKDF2(Username || Servername, "saltsalt", Nsalt, 8)
  std::unique_ptr<SymmetricKey> user_salt(SymmetricKey::DeriveKeyFromPassword(
      SymmetricKey::HMAC_SHA1, salt_password.str(), kSaltSalt, kSaltIterations,
      kSaltKeySizeInBits));
  DCHECK(user_salt);

  // Kuser = PBKDF2(P, Suser, Nuser, 16)
  user_key_ = SymmetricKey::DeriveKeyFromPassword(
      SymmetricKey::AES, password, user_salt->key(), kUserIterations,
      kDerivedKeySizeInBits);
  DCHECK(user_key_);

  // Kenc = PBKDF2(P, Suser, Nenc, 16)
  encryption_key_ = SymmetricKey::DeriveKeyFromPassword(
      SymmetricKey::AES, password, user_salt->key(), kEncryptionIterations,
      kDerivedKeySizeInBits);
  DCHECK(encryption_key_);

  // Kmac = PBKDF2(P, Suser, Nmac, 16)
  mac_key_ = SymmetricKey::DeriveKeyFromPassword(
      SymmetricKey::HMAC_SHA1, password, user_salt->key(), kSigningIterations,
      kDerivedKeySizeInBits);
  DCHECK(mac_key_);

  return user_key_ && encryption_key_ && mac_key_;
}

bool Nigori::InitByImport(const std::string& user_key,
                          const std::string& encryption_key,
                          const std::string& mac_key) {
  user_key_ = SymmetricKey::Import(SymmetricKey::AES, user_key);

  encryption_key_ = SymmetricKey::Import(SymmetricKey::AES, encryption_key);
  DCHECK(encryption_key_);

  mac_key_ = SymmetricKey::Import(SymmetricKey::HMAC_SHA1, mac_key);
  DCHECK(mac_key_);

  return encryption_key_ && mac_key_;
}

// Permute[Kenc,Kmac](type || name)
bool Nigori::Permute(Type type,
                     const std::string& name,
                     std::string* permuted) const {
  DCHECK_LT(0U, name.size());

  NigoriStream plaintext;
  plaintext << type << name;

  crypto::Encryptor encryptor;
  if (!encryptor.Init(encryption_key_.get(), crypto::Encryptor::CBC,
                      std::string(kIvSize, 0)))
    return false;

  std::string ciphertext;
  if (!encryptor.Encrypt(plaintext.str(), &ciphertext))
    return false;

  HMAC hmac(HMAC::SHA256);
  if (!hmac.Init(mac_key_->key()))
    return false;

  std::vector<unsigned char> hash(kHashSize);
  if (!hmac.Sign(ciphertext, &hash[0], hash.size()))
    return false;

  std::string output;
  output.assign(ciphertext);
  output.append(hash.begin(), hash.end());

  Base64Encode(output, permuted);
  return true;
}

// Enc[Kenc,Kmac](value)
bool Nigori::Encrypt(const std::string& value, std::string* encrypted) const {
  if (0U >= value.size())
    return false;

  std::string iv;
  crypto::RandBytes(base::WriteInto(&iv, kIvSize + 1), kIvSize);

  crypto::Encryptor encryptor;
  if (!encryptor.Init(encryption_key_.get(), crypto::Encryptor::CBC, iv))
    return false;

  std::string ciphertext;
  if (!encryptor.Encrypt(value, &ciphertext))
    return false;

  HMAC hmac(HMAC::SHA256);
  if (!hmac.Init(mac_key_->key()))
    return false;

  std::vector<unsigned char> hash(kHashSize);
  if (!hmac.Sign(ciphertext, &hash[0], hash.size()))
    return false;

  std::string output;
  output.assign(iv);
  output.append(ciphertext);
  output.append(hash.begin(), hash.end());

  Base64Encode(output, encrypted);
  return true;
}

bool Nigori::Decrypt(const std::string& encrypted, std::string* value) const {
  std::string input;
  if (!Base64Decode(encrypted, &input))
    return false;

  if (input.size() < kIvSize * 2 + kHashSize)
    return false;

  // The input is:
  // * iv (16 bytes)
  // * ciphertext (multiple of 16 bytes)
  // * hash (32 bytes)
  std::string iv(input.substr(0, kIvSize));
  std::string ciphertext(
      input.substr(kIvSize, input.size() - (kIvSize + kHashSize)));
  std::string hash(input.substr(input.size() - kHashSize, kHashSize));

  HMAC hmac(HMAC::SHA256);
  if (!hmac.Init(mac_key_->key()))
    return false;

  std::vector<unsigned char> expected(kHashSize);
  if (!hmac.Sign(ciphertext, &expected[0], expected.size()))
    return false;

  if (hash.compare(0, hash.size(), reinterpret_cast<char*>(&expected[0]),
                   expected.size()))
    return false;

  crypto::Encryptor encryptor;
  if (!encryptor.Init(encryption_key_.get(), crypto::Encryptor::CBC, iv))
    return false;

  if (!encryptor.Decrypt(ciphertext, value))
    return false;

  return true;
}

void Nigori::ExportKeys(std::string* user_key,
                        std::string* encryption_key,
                        std::string* mac_key) const {
  DCHECK(encryption_key);
  DCHECK(mac_key);
  DCHECK(user_key);

  if (user_key_)
    *user_key = user_key_->key();
  else
    user_key->clear();

  *encryption_key = encryption_key_->key();
  *mac_key = mac_key_->key();
}

}  // namespace syncer
