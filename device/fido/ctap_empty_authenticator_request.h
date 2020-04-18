// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_CTAP_EMPTY_AUTHENTICATOR_REQUEST_H_
#define DEVICE_FIDO_CTAP_EMPTY_AUTHENTICATOR_REQUEST_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/component_export.h"
#include "device/fido/fido_constants.h"

namespace device {

namespace internal {

// Represents CTAP requests with empty parameters, including
// AuthenticatorGetInfo, AuthenticatorCancel, AuthenticatorReset and
// AuthenticatorGetNextAssertion commands.
class COMPONENT_EXPORT(DEVICE_FIDO) CtapEmptyAuthenticatorRequest {
 public:
  CtapRequestCommand cmd() const { return cmd_; }
  std::vector<uint8_t> Serialize() const;

 protected:
  explicit CtapEmptyAuthenticatorRequest(CtapRequestCommand cmd) : cmd_(cmd) {}

 private:
  CtapRequestCommand cmd_;
};

}  // namespace internal

class COMPONENT_EXPORT(DEVICE_FIDO) AuthenticatorGetNextAssertionRequest
    : public internal::CtapEmptyAuthenticatorRequest {
 public:
  AuthenticatorGetNextAssertionRequest()
      : CtapEmptyAuthenticatorRequest(
            CtapRequestCommand::kAuthenticatorGetNextAssertion) {}
};

class COMPONENT_EXPORT(DEVICE_FIDO) AuthenticatorGetInfoRequest
    : public internal::CtapEmptyAuthenticatorRequest {
 public:
  AuthenticatorGetInfoRequest()
      : CtapEmptyAuthenticatorRequest(
            CtapRequestCommand::kAuthenticatorGetInfo) {}
};

class COMPONENT_EXPORT(DEVICE_FIDO) AuthenticatorResetRequest
    : public internal::CtapEmptyAuthenticatorRequest {
 public:
  AuthenticatorResetRequest()
      : CtapEmptyAuthenticatorRequest(CtapRequestCommand::kAuthenticatorReset) {
  }
};

}  // namespace device

#endif  // DEVICE_FIDO_CTAP_EMPTY_AUTHENTICATOR_REQUEST_H_
