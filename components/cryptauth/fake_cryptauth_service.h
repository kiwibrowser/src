// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_FAKE_CRYPTAUTH_SERVICE_H_
#define COMPONENTS_CRYPTAUTH_FAKE_CRYPTAUTH_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "components/cryptauth/cryptauth_service.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {

class CryptAuthClientFactory;
class CryptAuthDeviceManager;
class CryptAuthEnrollmentManager;

// Service which provides access to various CryptAuth singletons.
class FakeCryptAuthService : public CryptAuthService {
 public:
  FakeCryptAuthService();
  ~FakeCryptAuthService() override;

  void set_cryptauth_device_manager(
      CryptAuthDeviceManager* cryptauth_device_manager) {
    cryptauth_device_manager_ = cryptauth_device_manager;
  }

  void set_cryptauth_enrollment_manager(
      CryptAuthEnrollmentManager* cryptauth_enrollment_manager) {
    cryptauth_enrollment_manager_ = cryptauth_enrollment_manager;
  }

  void set_device_classifier(const DeviceClassifier& device_classifier) {
    device_classifier_ = device_classifier;
  }

  void set_account_id(const std::string& account_id) {
    account_id_ = account_id;
  }

  // CryptAuthService:
  CryptAuthDeviceManager* GetCryptAuthDeviceManager() override;
  CryptAuthEnrollmentManager* GetCryptAuthEnrollmentManager() override;
  DeviceClassifier GetDeviceClassifier() override;
  std::string GetAccountId() override;
  std::unique_ptr<CryptAuthClientFactory> CreateCryptAuthClientFactory()
      override;

 private:
  CryptAuthDeviceManager* cryptauth_device_manager_;
  CryptAuthEnrollmentManager* cryptauth_enrollment_manager_;
  DeviceClassifier device_classifier_;
  std::string account_id_;

  DISALLOW_COPY_AND_ASSIGN(FakeCryptAuthService);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_FAKE_CRYPTAUTH_SERVICE_H_
