// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_MAC_UTIL_H_
#define DEVICE_FIDO_MAC_UTIL_H_

#include <string>
#include <vector>

#import <Security/Security.h>

#include "base/callback.h"
#include "base/mac/availability.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_nsobject.h"

namespace device {
namespace fido {
namespace mac {

// KeychainItemIdentifier returns the unique identifier for a key pair, derived
// from an RP ID and user handle. It is stored in the keychain items
// kSecAttrApplicationLabel attribute and can be used for lookup.
std::vector<uint8_t> KeychainItemIdentifier(std::string rp_id,
                                            std::vector<uint8_t> user_id);

// MakeAuthenticatorData returns an AuthenticatorData instance for the Touch ID
// authenticator with the given Relying Party ID, credential ID and public key.
// It returns |base::nullopt| on failure.
base::Optional<AuthenticatorData> MakeAuthenticatorData(
    const std::string& rp_id,
    std::vector<uint8_t> credential_id,
    SecKeyRef public_key) API_AVAILABLE(macosx(10.12.2));

// GenerateSignature signs the concatenation of the serialization of the given
// authenticator data and the given client data hash, as required for
// (self-)attestation and assertion. Returns |base::nullopt| if the operation
// fails.
base::Optional<std::vector<uint8_t>> GenerateSignature(
    const AuthenticatorData& authenticator_data,
    const std::vector<uint8_t>& client_data_hash,
    SecKeyRef private_key) API_AVAILABLE(macosx(10.12.2));

std::vector<uint8_t> TouchIdAaguid();

}  // namespace mac
}  // namespace fido
}  // namespace device

#endif  // DEVICE_FIDO_MAC_UTIL_H_
