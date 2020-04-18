// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/attestation_object.h"

#include <utility>

#include "components/cbor/cbor_values.h"
#include "components/cbor/cbor_writer.h"
#include "device/fido/attestation_statement.h"
#include "device/fido/fido_constants.h"

namespace device {

AttestationObject::AttestationObject(
    AuthenticatorData data,
    std::unique_ptr<AttestationStatement> statement)
    : authenticator_data_(std::move(data)),
      attestation_statement_(std::move(statement)) {}

AttestationObject::AttestationObject(AttestationObject&& other) = default;
AttestationObject& AttestationObject::operator=(AttestationObject&& other) =
    default;

AttestationObject::~AttestationObject() = default;

std::vector<uint8_t> AttestationObject::GetCredentialId() const {
  return authenticator_data_.GetCredentialId();
}

void AttestationObject::EraseAttestationStatement() {
  attestation_statement_ = std::make_unique<NoneAttestationStatement>();
  authenticator_data_.DeleteDeviceAaguid();

// Attested credential data is optional section within authenticator data. But
// if present, the first 16 bytes of it represents a device AAGUID which must
// be set to zeros for none attestation statement format.
#if DCHECK_IS_ON()
  if (!authenticator_data_.attested_data())
    return;

  std::vector<uint8_t> auth_data = authenticator_data_.SerializeToByteArray();
  // See diagram at https://w3c.github.io/webauthn/#sctn-attestation
  constexpr size_t kAaguidOffset =
      32 /* RP ID hash */ + 1 /* flags */ + 4 /* signature counter */;
  constexpr size_t kAaguidSize = 16;
  DCHECK_GE(auth_data.size(), kAaguidOffset + kAaguidSize);
  DCHECK(std::all_of(auth_data.data() + kAaguidOffset,
                     auth_data.data() + kAaguidOffset + kAaguidSize,
                     [](uint8_t v) { return v == 0; }));
#endif
}

bool AttestationObject::IsAttestationCertificateInappropriatelyIdentifying() {
  return attestation_statement_
      ->IsAttestationCertificateInappropriatelyIdentifying();
}

std::vector<uint8_t> AttestationObject::SerializeToCBOREncodedBytes() const {
  cbor::CBORValue::MapValue map;
  map[cbor::CBORValue(kFormatKey)] =
      cbor::CBORValue(attestation_statement_->format_name());
  map[cbor::CBORValue(kAuthDataKey)] =
      cbor::CBORValue(authenticator_data_.SerializeToByteArray());
  map[cbor::CBORValue(kAttestationStatementKey)] =
      cbor::CBORValue(attestation_statement_->GetAsCBORMap());
  return cbor::CBORWriter::Write(cbor::CBORValue(std::move(map)))
      .value_or(std::vector<uint8_t>());
}

}  // namespace device
