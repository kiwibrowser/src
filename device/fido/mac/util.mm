// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/mac/get_assertion_operation.h"

#include <array>
#include <set>
#include <string>

#import <Foundation/Foundation.h>

#include "base/bind.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_logging.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/string_number_conversions.h"
#include "components/cbor/cbor_writer.h"
#include "device/fido/ec_public_key.h"
#include "device/fido/fido_attestation_statement.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_parsing_utils.h"
#include "device/fido/mac/keychain.h"

namespace device {
namespace fido {
namespace mac {

using base::ScopedCFTypeRef;
using base::scoped_nsobject;
using cbor::CBORWriter;
using cbor::CBORValue;

// The authenticator AAGUID value.
constexpr std::array<uint8_t, 16> kAaguid = {0xad, 0xce, 0x00, 0x02, 0x35, 0xbc,
                                             0xc6, 0x0a, 0x64, 0x8b, 0x0b, 0x25,
                                             0xf1, 0xf0, 0x55, 0x03};

std::vector<uint8_t> TouchIdAaguid() {
  return std::vector<uint8_t>(kAaguid.begin(), kAaguid.end());
}

namespace {

// MakeECPublicKey converts a SecKeyRef for a public key into an equivalent
// |ECPublicKey| instance. It returns |nullptr| if the key cannot be converted.
std::unique_ptr<ECPublicKey> MakeECPublicKey(SecKeyRef public_key_ref)
    API_AVAILABLE(macosx(10.12.2)) {
  CHECK(public_key_ref);
  ScopedCFTypeRef<CFErrorRef> err;
  ScopedCFTypeRef<CFDataRef> data_ref(
      SecKeyCopyExternalRepresentation(public_key_ref, err.InitializeInto()));
  if (!data_ref) {
    LOG(ERROR) << "SecCopyExternalRepresentation failed: " << err;
    return nullptr;
  }
  base::span<const uint8_t> key_data =
      base::make_span(CFDataGetBytePtr(data_ref), CFDataGetLength(data_ref));
  auto key =
      ECPublicKey::ParseX962Uncompressed(fido_parsing_utils::kEs256, key_data);
  if (!key) {
    LOG(ERROR) << "Unexpected public key format: "
               << base::HexEncode(key_data.data(), key_data.size());
    return nullptr;
  }
  return key;
}

}  // namespace

// KeychainItemIdentifier returns the unique identifier for a given RP ID
// and user handle. It is stored in the keychain items Application Label and
// used for later lookup.
std::vector<uint8_t> KeychainItemIdentifier(std::string rp_id,
                                            std::vector<uint8_t> user_id) {
  std::vector<CBORValue> array;
  array.emplace_back(CBORValue(rp_id));
  array.emplace_back(CBORValue(user_id));
  auto value = CBORWriter::Write(CBORValue(std::move(array)));
  CHECK(value);
  return *value;
}

base::Optional<AuthenticatorData> MakeAuthenticatorData(
    const std::string& rp_id,
    std::vector<uint8_t> credential_id,
    SecKeyRef public_key) API_AVAILABLE(macosx(10.12.2)) {
  if (credential_id.size() > 255) {
    LOG(ERROR) << "credential id too long: "
               << base::HexEncode(credential_id.data(), credential_id.size());
    return base::nullopt;
  }
  std::array<uint8_t, 2> encoded_credential_id_length = {
      0, static_cast<uint8_t>(credential_id.size())};
  constexpr uint8_t flags =
      static_cast<uint8_t>(AuthenticatorData::Flag::kTestOfUserVerification) |
      static_cast<uint8_t>(AuthenticatorData::Flag::kAttestation);
  std::vector<uint8_t> counter = {0, 0, 0, 0};  // implement
  auto ec_public_key = MakeECPublicKey(public_key);
  if (!ec_public_key) {
    LOG(ERROR) << "MakeECPublicKey failed";
    return base::nullopt;
  }
  return AuthenticatorData(
      fido_parsing_utils::CreateSHA256Hash(rp_id), flags, counter,
      AttestedCredentialData(kAaguid, encoded_credential_id_length,
                             std::move(credential_id),
                             std::move(ec_public_key)));
}

base::Optional<std::vector<uint8_t>> GenerateSignature(
    const AuthenticatorData& authenticator_data,
    const std::vector<uint8_t>& client_data_hash,
    SecKeyRef private_key) API_AVAILABLE(macosx(10.12.2)) {
  const std::vector<uint8_t> serialized_authenticator_data =
      authenticator_data.SerializeToByteArray();
  size_t capacity =
      serialized_authenticator_data.size() + client_data_hash.size();
  ScopedCFTypeRef<CFMutableDataRef> sig_input(
      CFDataCreateMutable(kCFAllocatorDefault, capacity));
  CFDataAppendBytes(sig_input, serialized_authenticator_data.data(),
                    serialized_authenticator_data.size());
  CFDataAppendBytes(sig_input, client_data_hash.data(),
                    client_data_hash.size());
  ScopedCFTypeRef<CFErrorRef> err;
  ScopedCFTypeRef<CFDataRef> sig_data(
      Keychain::GetInstance().KeyCreateSignature(
          private_key, kSecKeyAlgorithmECDSASignatureMessageX962SHA256,
          sig_input, err.InitializeInto()));
  if (!sig_data) {
    LOG(ERROR) << "SecKeyCreateSignature failed: " << err;
    return base::nullopt;
  }
  return std::vector<uint8_t>(
      CFDataGetBytePtr(sig_data),
      CFDataGetBytePtr(sig_data) + CFDataGetLength(sig_data));
}

}  // namespace mac
}  // namespace fido
}  // namespace device
