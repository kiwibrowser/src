// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/settings/device_identity_provider.h"

#include "chrome/browser/chromeos/settings/device_oauth2_token_service.h"

namespace chromeos {

DeviceIdentityProvider::DeviceIdentityProvider(
    chromeos::DeviceOAuth2TokenService* token_service)
    : token_service_(token_service) {}

DeviceIdentityProvider::~DeviceIdentityProvider() {}

std::string DeviceIdentityProvider::GetActiveUsername() {
  return token_service_->GetRobotAccountId();
}

std::string DeviceIdentityProvider::GetActiveAccountId() {
  return token_service_->GetRobotAccountId();
}

OAuth2TokenService* DeviceIdentityProvider::GetTokenService() {
  return token_service_;
}

}  // namespace chromeos
