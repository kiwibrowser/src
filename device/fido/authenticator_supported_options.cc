// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/authenticator_supported_options.h"

namespace device {

AuthenticatorSupportedOptions::AuthenticatorSupportedOptions() = default;

AuthenticatorSupportedOptions::AuthenticatorSupportedOptions(
    AuthenticatorSupportedOptions&& other) = default;

AuthenticatorSupportedOptions& AuthenticatorSupportedOptions::operator=(
    AuthenticatorSupportedOptions&& other) = default;

AuthenticatorSupportedOptions::~AuthenticatorSupportedOptions() = default;

AuthenticatorSupportedOptions&
AuthenticatorSupportedOptions::SetIsPlatformDevice(bool is_platform_device) {
  is_platform_device_ = is_platform_device;
  return *this;
}

AuthenticatorSupportedOptions&
AuthenticatorSupportedOptions::SetSupportsResidentKey(
    bool supports_resident_key) {
  supports_resident_key_ = supports_resident_key;
  return *this;
}

AuthenticatorSupportedOptions&
AuthenticatorSupportedOptions::SetUserVerificationAvailability(
    UserVerificationAvailability user_verification_availability) {
  user_verification_availability_ = user_verification_availability;
  return *this;
}

AuthenticatorSupportedOptions&
AuthenticatorSupportedOptions::SetUserPresenceRequired(
    bool user_presence_required) {
  user_presence_required_ = user_presence_required;
  return *this;
}

AuthenticatorSupportedOptions&
AuthenticatorSupportedOptions::SetClientPinAvailability(
    ClientPinAvailability client_pin_availability) {
  client_pin_availability_ = client_pin_availability;
  return *this;
}

}  // namespace device
