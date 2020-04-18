// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_MAC_AUTHENTICATOR_H_
#define DEVICE_FIDO_MAC_AUTHENTICATOR_H_

#include "base/mac/availability.h"
#include "base/macros.h"
#include "base/strings/string_piece_forward.h"
#include "device/fido/fido_authenticator.h"
#include "device/fido/mac/operation.h"

namespace device {
namespace fido {
namespace mac {

class API_AVAILABLE(macosx(10.12.2)) TouchIdAuthenticator
    : public FidoAuthenticator {
 public:
  // IsAvailable returns true iff Touch ID is enabled and enrolled on the
  // current device.
  static bool IsAvailable();

  // CreateIfAvailable returns a TouchIdAuthenticator if IsAvailable() returns
  // true and nullptr otherwise.
  static std::unique_ptr<TouchIdAuthenticator> CreateIfAvailable();

  ~TouchIdAuthenticator() override;

  // TouchIdAuthenticator
  void MakeCredential(
      AuthenticatorSelectionCriteria authenticator_selection_criteria,
      CtapMakeCredentialRequest request,
      MakeCredentialCallback callback) override;
  void GetAssertion(CtapGetAssertionRequest request,
                    GetAssertionCallback callback) override;
  void Cancel() override;

  std::string GetId() const override;

 private:
  TouchIdAuthenticator();

  // The profile ID identifies the user profile from which the request
  // originates. It is used to scope credentials to the profile under which they
  // were created.
  base::StringPiece GetOrInitializeProfileId();

  // The keychain access group is a string value related to the Apple developer
  // ID under which the binary gets signed that the Keychain Services API use
  // for access control. See
  // https://developer.apple.com/documentation/security/ksecattraccessgroup?language=objc.
  base::StringPiece keychain_access_group() {
    return "EQHXZ8M8AV.com.google.chrome.webauthn";
  }

  std::unique_ptr<Operation> operation_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TouchIdAuthenticator);
};

}  // namespace mac
}  // namespace fido
}  // namespace device

#endif  // DEVICE_FIDO_MAC_AUTHENTICATOR_H_
