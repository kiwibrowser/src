// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_DEVICE_SYNC_CRYPTAUTH_ENROLLER_FACTORY_IMPL_H_
#define CHROMEOS_SERVICES_DEVICE_SYNC_CRYPTAUTH_ENROLLER_FACTORY_IMPL_H_

#include "components/cryptauth/cryptauth_enroller.h"

namespace cryptauth {
class CryptAuthClientFactory;
}  // namespace cryptauth

namespace chromeos {

namespace device_sync {

// CryptAuthEnrollerFactory implementation which utilizes IdentityManager.
class CryptAuthEnrollerFactoryImpl
    : public cryptauth::CryptAuthEnrollerFactory {
 public:
  CryptAuthEnrollerFactoryImpl(
      cryptauth::CryptAuthClientFactory* cryptauth_client_factory);
  ~CryptAuthEnrollerFactoryImpl() override;

  // cryptauth::CryptAuthEnrollerFactory:
  std::unique_ptr<cryptauth::CryptAuthEnroller> CreateInstance() override;

 private:
  cryptauth::CryptAuthClientFactory* cryptauth_client_factory_;
};

}  // namespace device_sync

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_DEVICE_SYNC_CRYPTAUTH_ENROLLER_FACTORY_IMPL_H_
