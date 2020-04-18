// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/opaque_attestation_statement.h"

#include <utility>

#include "components/cbor/cbor_values.h"

namespace device {

OpaqueAttestationStatement::OpaqueAttestationStatement(
    std::string attestation_format,
    cbor::CBORValue attestation_statement_map)
    : AttestationStatement(std::move(attestation_format)),
      attestation_statement_map_(std::move(attestation_statement_map)) {}

OpaqueAttestationStatement::~OpaqueAttestationStatement() = default;

// Returns the deep copied cbor map value of |attestation_statement_map_|.
cbor::CBORValue::MapValue OpaqueAttestationStatement::GetAsCBORMap() const {
  DCHECK(attestation_statement_map_.is_map());
  cbor::CBORValue::MapValue new_map;
  new_map.reserve(attestation_statement_map_.GetMap().size());
  for (const auto& map_it : attestation_statement_map_.GetMap()) {
    new_map.try_emplace(new_map.end(), map_it.first.Clone(),
                        map_it.second.Clone());
  }
  return new_map;
}

bool OpaqueAttestationStatement::
    IsAttestationCertificateInappropriatelyIdentifying() {
  return false;
}

}  // namespace device
