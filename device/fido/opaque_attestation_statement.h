// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_OPAQUE_ATTESTATION_STATEMENT_H_
#define DEVICE_FIDO_OPAQUE_ATTESTATION_STATEMENT_H_

#include <string>

#include "base/component_export.h"
#include "base/macros.h"
#include "components/cbor/cbor_values.h"
#include "device/fido/attestation_statement.h"

namespace device {

// https://www.w3.org/TR/2017/WD-webauthn-20170505/#fido-u2f-attestation
class COMPONENT_EXPORT(DEVICE_FIDO) OpaqueAttestationStatement
    : public AttestationStatement {
 public:
  OpaqueAttestationStatement(std::string attestation_format,
                             cbor::CBORValue attestation_statement_map);
  ~OpaqueAttestationStatement() override;

  // AttestationStatement:
  cbor::CBORValue::MapValue GetAsCBORMap() const override;
  bool IsAttestationCertificateInappropriatelyIdentifying() override;

 private:
  cbor::CBORValue attestation_statement_map_;

  DISALLOW_COPY_AND_ASSIGN(OpaqueAttestationStatement);
};

}  // namespace device

#endif  // DEVICE_FIDO_OPAQUE_ATTESTATION_STATEMENT_H_
