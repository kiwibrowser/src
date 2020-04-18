// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/authenticator_get_info_response.h"

#include <utility>

namespace device {

AuthenticatorGetInfoResponse::AuthenticatorGetInfoResponse(
    std::vector<std::string> versions,
    std::vector<uint8_t> aaguid)
    : versions_(std::move(versions)), aaguid_(std::move(aaguid)) {}

AuthenticatorGetInfoResponse::AuthenticatorGetInfoResponse(
    AuthenticatorGetInfoResponse&& that) = default;

AuthenticatorGetInfoResponse& AuthenticatorGetInfoResponse::operator=(
    AuthenticatorGetInfoResponse&& other) = default;

AuthenticatorGetInfoResponse::~AuthenticatorGetInfoResponse() = default;

AuthenticatorGetInfoResponse& AuthenticatorGetInfoResponse::SetMaxMsgSize(
    uint8_t max_msg_size) {
  max_msg_size_ = max_msg_size;
  return *this;
}

AuthenticatorGetInfoResponse& AuthenticatorGetInfoResponse::SetPinProtocols(
    std::vector<uint8_t> pin_protocols) {
  pin_protocols_ = std::move(pin_protocols);
  return *this;
}

AuthenticatorGetInfoResponse& AuthenticatorGetInfoResponse::SetExtensions(
    std::vector<std::string> extensions) {
  extensions_ = std::move(extensions);
  return *this;
}

AuthenticatorGetInfoResponse& AuthenticatorGetInfoResponse::SetOptions(
    AuthenticatorSupportedOptions options) {
  options_ = std::move(options);
  return *this;
}

}  // namespace device
