// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_DEVICE_RESPONSE_CONVERTER_H_
#define DEVICE_FIDO_DEVICE_RESPONSE_CONVERTER_H_

#include <stdint.h>

#include <vector>

#include "base/component_export.h"
#include "base/optional.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/authenticator_get_info_response.h"
#include "device/fido/authenticator_make_credential_response.h"
#include "device/fido/fido_constants.h"

// Converts response from authenticators to CTAPResponse objects. If the
// response of the authenticator does not conform to format specified by the
// CTAP protocol, null optional is returned.
namespace device {

// Parses response code from response received from the authenticator. If
// unknown response code value is received, then CTAP2_ERR_OTHER is returned.
COMPONENT_EXPORT(DEVICE_FIDO)
CtapDeviceResponseCode GetResponseCode(base::span<const uint8_t> buffer);

// De-serializes CBOR encoded response, checks for valid CBOR map formatting,
// and converts response to AuthenticatorMakeCredentialResponse object with
// CBOR map keys that conform to format of attestation object defined by the
// WebAuthN spec : https://w3c.github.io/webauthn/#fig-attStructs
COMPONENT_EXPORT(DEVICE_FIDO)
base::Optional<AuthenticatorMakeCredentialResponse>
ReadCTAPMakeCredentialResponse(base::span<const uint8_t> buffer);

// De-serializes CBOR encoded response to AuthenticatorGetAssertion /
// AuthenticatorGetNextAssertion request to AuthenticatorGetAssertionResponse
// object.
COMPONENT_EXPORT(DEVICE_FIDO)
base::Optional<AuthenticatorGetAssertionResponse> ReadCTAPGetAssertionResponse(
    base::span<const uint8_t> buffer);

// De-serializes CBOR encoded response to AuthenticatorGetInfo request to
// AuthenticatorGetInfoResponse object.
COMPONENT_EXPORT(DEVICE_FIDO)
base::Optional<AuthenticatorGetInfoResponse> ReadCTAPGetInfoResponse(
    base::span<const uint8_t> buffer);

}  // namespace device

#endif  // DEVICE_FIDO_DEVICE_RESPONSE_CONVERTER_H_
