// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_AUTHENTICATOR_SELECTION_CRITERIA_H_
#define DEVICE_FIDO_AUTHENTICATOR_SELECTION_CRITERIA_H_

#include "base/component_export.h"
#include "device/fido/fido_constants.h"

namespace device {

// Represents authenticator properties the relying party can specify to restrict
// the type of authenticator used in creating credentials.
// https://w3c.github.io/webauthn/#authenticatorSelection
class COMPONENT_EXPORT(DEVICE_FIDO) AuthenticatorSelectionCriteria {
 public:
  enum class AuthenticatorAttachment {
    kAny,
    kPlatform,
    kCrossPlatform,
  };

  AuthenticatorSelectionCriteria();
  AuthenticatorSelectionCriteria(
      AuthenticatorAttachment authenticator_attachement,
      bool require_resident_key,
      UserVerificationRequirement user_verification_requirement);
  AuthenticatorSelectionCriteria(const AuthenticatorSelectionCriteria& other);
  AuthenticatorSelectionCriteria(AuthenticatorSelectionCriteria&& other);
  AuthenticatorSelectionCriteria& operator=(
      const AuthenticatorSelectionCriteria& other);
  AuthenticatorSelectionCriteria& operator=(
      AuthenticatorSelectionCriteria&& other);
  ~AuthenticatorSelectionCriteria();

  AuthenticatorAttachment authenticator_attachement() const {
    return authenticator_attachement_;
  }

  bool require_resident_key() const { return require_resident_key_; }

  UserVerificationRequirement user_verification_requirement() const {
    return user_verification_requirement_;
  }

 private:
  AuthenticatorAttachment authenticator_attachement_ =
      AuthenticatorAttachment::kAny;
  bool require_resident_key_ = false;
  UserVerificationRequirement user_verification_requirement_ =
      UserVerificationRequirement::kPreferred;
};

}  // namespace device

#endif  // DEVICE_FIDO_AUTHENTICATOR_SELECTION_CRITERIA_H_
