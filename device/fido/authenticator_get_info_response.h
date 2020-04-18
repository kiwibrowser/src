// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_AUTHENTICATOR_GET_INFO_RESPONSE_H_
#define DEVICE_FIDO_AUTHENTICATOR_GET_INFO_RESPONSE_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/optional.h"
#include "device/fido/authenticator_supported_options.h"
#include "device/fido/fido_constants.h"

namespace device {

// Authenticator response for AuthenticatorGetInfo request that encapsulates
// versions, options, AAGUID(Authenticator Attestation GUID), other
// authenticator device information.
// https://fidoalliance.org/specs/fido-v2.0-rd-20170927/fido-client-to-authenticator-protocol-v2.0-rd-20170927.html#authenticatorGetInfo
class COMPONENT_EXPORT(DEVICE_FIDO) AuthenticatorGetInfoResponse {
 public:
  AuthenticatorGetInfoResponse(std::vector<std::string> versions,
                               std::vector<uint8_t> aaguid);
  AuthenticatorGetInfoResponse(AuthenticatorGetInfoResponse&& that);
  AuthenticatorGetInfoResponse& operator=(AuthenticatorGetInfoResponse&& other);
  ~AuthenticatorGetInfoResponse();

  AuthenticatorGetInfoResponse& SetMaxMsgSize(uint8_t max_msg_size);
  AuthenticatorGetInfoResponse& SetPinProtocols(
      std::vector<uint8_t> pin_protocols);
  AuthenticatorGetInfoResponse& SetExtensions(
      std::vector<std::string> extensions);
  AuthenticatorGetInfoResponse& SetOptions(
      AuthenticatorSupportedOptions options);

  const std::vector<std::string>& versions() { return versions_; }
  const std::vector<uint8_t>& aaguid() const { return aaguid_; }
  const base::Optional<uint8_t>& max_msg_size() const { return max_msg_size_; }
  const base::Optional<std::vector<uint8_t>>& pin_protocol() const {
    return pin_protocols_;
  }
  const base::Optional<std::vector<std::string>>& extensions() const {
    return extensions_;
  }
  const AuthenticatorSupportedOptions& options() const { return options_; }

 private:
  std::vector<std::string> versions_;
  std::vector<uint8_t> aaguid_;
  base::Optional<uint8_t> max_msg_size_;
  base::Optional<std::vector<uint8_t>> pin_protocols_;
  base::Optional<std::vector<std::string>> extensions_;
  AuthenticatorSupportedOptions options_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorGetInfoResponse);
};

}  // namespace device

#endif  // DEVICE_FIDO_AUTHENTICATOR_GET_INFO_RESPONSE_H_
