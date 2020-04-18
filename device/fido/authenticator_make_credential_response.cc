// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/authenticator_make_credential_response.h"

#include <utility>

#include "device/fido/attestation_object.h"
#include "device/fido/attested_credential_data.h"
#include "device/fido/authenticator_data.h"
#include "device/fido/ec_public_key.h"
#include "device/fido/fido_attestation_statement.h"
#include "device/fido/fido_parsing_utils.h"

namespace device {

// static
base::Optional<AuthenticatorMakeCredentialResponse>
AuthenticatorMakeCredentialResponse::CreateFromU2fRegisterResponse(
    const std::vector<uint8_t>& relying_party_id_hash,
    base::span<const uint8_t> u2f_data) {
  auto public_key = ECPublicKey::ExtractFromU2fRegistrationResponse(
      fido_parsing_utils::kEs256, u2f_data);
  if (!public_key)
    return base::nullopt;

  auto attested_credential_data =
      AttestedCredentialData::CreateFromU2fRegisterResponse(
          u2f_data, std::move(public_key));

  if (!attested_credential_data)
    return base::nullopt;

  // Extract the credential_id for packing into the response data.
  std::vector<uint8_t> credential_id =
      attested_credential_data->credential_id();

  // The counter is zeroed out for Register requests.
  std::vector<uint8_t> counter(4u);
  constexpr uint8_t flags =
      static_cast<uint8_t>(AuthenticatorData::Flag::kTestOfUserPresence) |
      static_cast<uint8_t>(AuthenticatorData::Flag::kAttestation);

  AuthenticatorData authenticator_data(std::move(relying_party_id_hash), flags,
                                       std::move(counter),
                                       std::move(attested_credential_data));

  auto fido_attestation_statement =
      FidoAttestationStatement::CreateFromU2fRegisterResponse(u2f_data);

  if (!fido_attestation_statement)
    return base::nullopt;

  return AuthenticatorMakeCredentialResponse(AttestationObject(
      std::move(authenticator_data), std::move(fido_attestation_statement)));
}

AuthenticatorMakeCredentialResponse::AuthenticatorMakeCredentialResponse(
    AttestationObject attestation_object)
    : ResponseData(attestation_object.GetCredentialId()),
      attestation_object_(std::move(attestation_object)) {}

AuthenticatorMakeCredentialResponse::AuthenticatorMakeCredentialResponse(
    AuthenticatorMakeCredentialResponse&& that) = default;

AuthenticatorMakeCredentialResponse& AuthenticatorMakeCredentialResponse::
operator=(AuthenticatorMakeCredentialResponse&& other) = default;

AuthenticatorMakeCredentialResponse::~AuthenticatorMakeCredentialResponse() =
    default;

std::vector<uint8_t>
AuthenticatorMakeCredentialResponse::GetCBOREncodedAttestationObject() const {
  return attestation_object_.SerializeToCBOREncodedBytes();
}

void AuthenticatorMakeCredentialResponse::EraseAttestationStatement() {
  attestation_object_.EraseAttestationStatement();
}

bool AuthenticatorMakeCredentialResponse::
    IsAttestationCertificateInappropriatelyIdentifying() {
  return attestation_object_
      .IsAttestationCertificateInappropriatelyIdentifying();
}

const std::vector<uint8_t>& AuthenticatorMakeCredentialResponse::GetRpIdHash()
    const {
  return attestation_object_.rp_id_hash();
}

}  // namespace device
