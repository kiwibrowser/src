// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_ATTESTATION_STATEMENT_H_
#define DEVICE_FIDO_FIDO_ATTESTATION_STATEMENT_H_

#include <stdint.h>
#include <memory>
#include <vector>

#include "base/component_export.h"
#include "base/containers/span.h"
#include "base/macros.h"
#include "components/cbor/cbor_values.h"
#include "device/fido/attestation_statement.h"

namespace device {

// https://www.w3.org/TR/2017/WD-webauthn-20170505/#fido-u2f-attestation
class COMPONENT_EXPORT(DEVICE_FIDO) FidoAttestationStatement
    : public AttestationStatement {
 public:
  static std::unique_ptr<FidoAttestationStatement>
  CreateFromU2fRegisterResponse(base::span<const uint8_t> u2f_data);

  FidoAttestationStatement(std::vector<uint8_t> signature,
                           std::vector<std::vector<uint8_t>> x509_certificates);
  ~FidoAttestationStatement() override;

  // AttestationStatement overrides

  // Produces a map in the following format:
  // { "x5c": [ x509_certs bytes ], "sig": signature bytes ] }
  cbor::CBORValue::MapValue GetAsCBORMap() const override;

  bool IsAttestationCertificateInappropriatelyIdentifying() override;

 private:
  const std::vector<uint8_t> signature_;
  const std::vector<std::vector<uint8_t>> x509_certificates_;

  DISALLOW_COPY_AND_ASSIGN(FidoAttestationStatement);
};

}  // namespace device

#endif  // DEVICE_FIDO_FIDO_ATTESTATION_STATEMENT_H_
