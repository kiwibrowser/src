// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_CRYPTAUTH_SERVICE_H_
#define COMPONENTS_CRYPTAUTH_CRYPTAUTH_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/prefs/pref_registry_simple.h"

namespace cryptauth {

class CryptAuthClientFactory;
class CryptAuthDeviceManager;
class CryptAuthEnrollmentManager;

// Service which provides access to various CryptAuth singletons.
class CryptAuthService {
 public:
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  virtual CryptAuthDeviceManager* GetCryptAuthDeviceManager() = 0;
  virtual CryptAuthEnrollmentManager* GetCryptAuthEnrollmentManager() = 0;
  virtual DeviceClassifier GetDeviceClassifier() = 0;
  virtual std::string GetAccountId() = 0;
  virtual std::unique_ptr<CryptAuthClientFactory>
  CreateCryptAuthClientFactory() = 0;

 protected:
  CryptAuthService() = default;
  virtual ~CryptAuthService() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(CryptAuthService);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_CRYPTAUTH_SERVICE_H_
