// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_NETWORKING_PRIVATE_NETWORKING_PRIVATE_CRYPTO_H_
#define CHROME_BROWSER_EXTENSIONS_API_NETWORKING_PRIVATE_NETWORKING_PRIVATE_CRYPTO_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/time/time.h"

namespace networking_private_crypto {

// Verify that the credentials described by |certificate| and |signed_data|
// are valid as follows:
// 1) The MAC address listed in the certificate matches |connected_mac|.
// 2) The certificate is a valid PEM encoded certificate signed by trusted CA.
// 3) |signature| is a valid signature for |data|, using the public key in
// |certificate|
bool VerifyCredentials(
    const std::string& certificate,
    const std::vector<std::string>& intermediate_certificates,
    const std::string& signature,
    const std::string& data,
    const std::string& connected_mac);

// The same as VerifyCredentials() above, but uses time |time| rather than the
// current time for checking validity.
bool VerifyCredentialsAtTime(
    const std::string& certificate,
    const std::vector<std::string>& intermediate_certificates,
    const std::string& signature,
    const std::string& data,
    const std::string& connected_mac,
    const base::Time& time);

// Encrypt |data| with |public_key|. |public_key| is a DER-encoded
// RSAPublicKey. |data| is some string of bytes that is smaller than the
// maximum length permissible for PKCS#1 v1.5 with a key of |public_key| size.
//
// Returns true on success, storing the encrypted result in
// |encrypted_output|.
bool EncryptByteString(const std::vector<uint8_t>& public_key,
                       const std::string& data,
                       std::vector<uint8_t>* encrypted_output);

// Decrypt |encrypted_data| with |private_key_pem|. |private_key_pem| is the
// PKCS8 PEM-encoded private key. |encrypted_data| is data encrypted with
// EncryptByteString. Used in NetworkingPrivateCryptoTest::EncryptString test.
// Returns true on success, storing the decrypted result in
// |decrypted_output|.
bool DecryptByteString(const std::string& private_key_pem,
                       const std::vector<uint8_t>& encrypted_data,
                       std::string* decrypted_output);

}  // namespace networking_private_crypto

#endif  // CHROME_BROWSER_EXTENSIONS_API_NETWORKING_PRIVATE_NETWORKING_PRIVATE_CRYPTO_H_
