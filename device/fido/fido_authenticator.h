// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_AUTHENTICATOR_H_
#define DEVICE_FIDO_FIDO_AUTHENTICATOR_H_

#include <string>

#include "base/callback_forward.h"
#include "base/component_export.h"
#include "base/macros.h"
#include "base/optional.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/authenticator_make_credential_response.h"

namespace device {

class AuthenticatorSelectionCriteria;
class CtapGetAssertionRequest;
class CtapMakeCredentialRequest;

// FidoAuthenticator is an authenticator from the WebAuthn Authenticator model
// (https://www.w3.org/TR/webauthn/#sctn-authenticator-model). It may be a
// physical device, or a built-in (platform) authenticator.
class COMPONENT_EXPORT(DEVICE_FIDO) FidoAuthenticator {
 public:
  using MakeCredentialCallback = base::OnceCallback<void(
      CtapDeviceResponseCode,
      base::Optional<AuthenticatorMakeCredentialResponse>)>;
  using GetAssertionCallback = base::OnceCallback<void(
      CtapDeviceResponseCode,
      base::Optional<AuthenticatorGetAssertionResponse>)>;

  FidoAuthenticator() = default;
  virtual ~FidoAuthenticator() = default;

  virtual void MakeCredential(
      AuthenticatorSelectionCriteria authenticator_selection_criteria,
      CtapMakeCredentialRequest request,
      MakeCredentialCallback callback) = 0;
  virtual void GetAssertion(CtapGetAssertionRequest request,
                            GetAssertionCallback callback) = 0;
  virtual void Cancel() = 0;
  virtual std::string GetId() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(FidoAuthenticator);
};

}  // namespace device

#endif  // DEVICE_FIDO_FIDO_AUTHENTICATOR_H_
