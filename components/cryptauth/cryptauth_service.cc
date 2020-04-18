// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/cryptauth_service.h"

#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/cryptauth_enrollment_manager.h"
#include "components/cryptauth/cryptauth_gcm_manager.h"

namespace cryptauth {

// static
void CryptAuthService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  CryptAuthGCMManager::RegisterPrefs(registry);
  CryptAuthDeviceManager::RegisterPrefs(registry);
  CryptAuthEnrollmentManager::RegisterPrefs(registry);
}

}  // namespace cryptauth
