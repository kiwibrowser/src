// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/virtual_fido_device.h"

#include <tuple>
#include <utility>

#include "crypto/ec_private_key.h"
#include "crypto/ec_signature_creator.h"
#include "device/fido/fido_parsing_utils.h"

namespace device {

namespace {

// The example attestation private key from the U2F spec at
// https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-raw-message-formats-v1.2-ps-20170411.html#registration-example
//
// PKCS.8 encoded without encryption.
constexpr uint8_t kAttestationKey[]{
    0x30, 0x81, 0x87, 0x02, 0x01, 0x00, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86,
    0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d,
    0x03, 0x01, 0x07, 0x04, 0x6d, 0x30, 0x6b, 0x02, 0x01, 0x01, 0x04, 0x20,
    0xf3, 0xfc, 0xcc, 0x0d, 0x00, 0xd8, 0x03, 0x19, 0x54, 0xf9, 0x08, 0x64,
    0xd4, 0x3c, 0x24, 0x7f, 0x4b, 0xf5, 0xf0, 0x66, 0x5c, 0x6b, 0x50, 0xcc,
    0x17, 0x74, 0x9a, 0x27, 0xd1, 0xcf, 0x76, 0x64, 0xa1, 0x44, 0x03, 0x42,
    0x00, 0x04, 0x8d, 0x61, 0x7e, 0x65, 0xc9, 0x50, 0x8e, 0x64, 0xbc, 0xc5,
    0x67, 0x3a, 0xc8, 0x2a, 0x67, 0x99, 0xda, 0x3c, 0x14, 0x46, 0x68, 0x2c,
    0x25, 0x8c, 0x46, 0x3f, 0xff, 0xdf, 0x58, 0xdf, 0xd2, 0xfa, 0x3e, 0x6c,
    0x37, 0x8b, 0x53, 0xd7, 0x95, 0xc4, 0xa4, 0xdf, 0xfb, 0x41, 0x99, 0xed,
    0xd7, 0x86, 0x2f, 0x23, 0xab, 0xaf, 0x02, 0x03, 0xb4, 0xb8, 0x91, 0x1b,
    0xa0, 0x56, 0x99, 0x94, 0xe1, 0x01};

}  // namespace

// VirtualFidoDevice::RegistrationData ----------------------------------------

VirtualFidoDevice::RegistrationData::RegistrationData() = default;
VirtualFidoDevice::RegistrationData::RegistrationData(
    std::unique_ptr<crypto::ECPrivateKey> private_key,
    std::vector<uint8_t> application_parameter,
    uint32_t counter)
    : private_key(std::move(private_key)),
      application_parameter(std::move(application_parameter)),
      counter(counter) {}
VirtualFidoDevice::RegistrationData::RegistrationData(RegistrationData&& data) =
    default;
VirtualFidoDevice::RegistrationData::~RegistrationData() = default;

VirtualFidoDevice::RegistrationData& VirtualFidoDevice::RegistrationData::
operator=(RegistrationData&& other) = default;

// VirtualFidoDevice::State ---------------------------------------------------

VirtualFidoDevice::State::State()
    : attestation_cert_common_name("Batch Certificate"),
      individual_attestation_cert_common_name("Individual Certificate") {}
VirtualFidoDevice::State::~State() = default;

bool VirtualFidoDevice::State::InjectRegistration(
    const std::vector<uint8_t>& credential_id,
    const std::string& relying_party_id) {
  auto application_parameter =
      fido_parsing_utils::CreateSHA256Hash(relying_party_id);
  auto private_key = crypto::ECPrivateKey::Create();
  if (!private_key)
    return false;

  RegistrationData registration(std::move(private_key),
                                std::move(application_parameter),
                                0 /* signature counter */);

  bool was_inserted;
  std::tie(std::ignore, was_inserted) =
      registrations.emplace(credential_id, std::move(registration));
  return was_inserted;
}

VirtualFidoDevice::VirtualFidoDevice() = default;

// VirtualFidoDevice ----------------------------------------------------------

VirtualFidoDevice::VirtualFidoDevice(scoped_refptr<State> state)
    : state_(std::move(state)) {}

VirtualFidoDevice::~VirtualFidoDevice() = default;

// static
std::vector<uint8_t> VirtualFidoDevice::GetAttestationKey() {
  return fido_parsing_utils::Materialize(kAttestationKey);
}

// static
bool VirtualFidoDevice::Sign(crypto::ECPrivateKey* private_key,
                             base::span<const uint8_t> sign_buffer,
                             std::vector<uint8_t>* signature) {
  auto signer = crypto::ECSignatureCreator::Create(private_key);
  return signer->Sign(sign_buffer.data(), sign_buffer.size(), signature);
}

base::Optional<std::vector<uint8_t>>
VirtualFidoDevice::GenerateAttestationCertificate(
    bool individual_attestation_requested) const {
  std::unique_ptr<crypto::ECPrivateKey> attestation_private_key =
      crypto::ECPrivateKey::CreateFromPrivateKeyInfo(GetAttestationKey());
  constexpr uint32_t kAttestationCertSerialNumber = 1;
  std::string attestation_cert;
  if (!net::x509_util::CreateSelfSignedCert(
          attestation_private_key->key(), net::x509_util::DIGEST_SHA256,
          "CN=" + (individual_attestation_requested
                       ? state_->individual_attestation_cert_common_name
                       : state_->attestation_cert_common_name),
          kAttestationCertSerialNumber, base::Time::FromTimeT(1500000000),
          base::Time::FromTimeT(1500000000), &attestation_cert)) {
    DVLOG(2) << "Failed to create attestation certificate";
    return base::nullopt;
  }

  return std::vector<uint8_t>(attestation_cert.begin(), attestation_cert.end());
}

void VirtualFidoDevice::TryWink(WinkCallback cb) {
  std::move(cb).Run();
}

std::string VirtualFidoDevice::GetId() const {
  // Use our heap address to get a unique-ish number. (0xffe1 is a prime).
  return "VirtualFidoDevice-" + std::to_string((size_t)this % 0xffe1);
}

}  // namespace device
