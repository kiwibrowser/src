// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/credentialmanager/authenticator_attestation_response.h"

namespace blink {

AuthenticatorAttestationResponse* AuthenticatorAttestationResponse::Create(
    DOMArrayBuffer* client_data_json,
    DOMArrayBuffer* attestation_object) {
  return new AuthenticatorAttestationResponse(client_data_json,
                                              attestation_object);
}

AuthenticatorAttestationResponse::AuthenticatorAttestationResponse(
    DOMArrayBuffer* client_data_json,
    DOMArrayBuffer* attestation_object)
    : AuthenticatorResponse(client_data_json),
      attestation_object_(attestation_object) {}

AuthenticatorAttestationResponse::~AuthenticatorAttestationResponse() = default;

void AuthenticatorAttestationResponse::Trace(blink::Visitor* visitor) {
  visitor->Trace(attestation_object_);
  AuthenticatorResponse::Trace(visitor);
}

}  // namespace blink
