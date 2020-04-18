// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/device_sync/cryptauth_enroller_factory_impl.h"

#include <memory>

#include "components/cryptauth/cryptauth_enroller_impl.h"
#include "components/cryptauth/secure_message_delegate_impl.h"

namespace chromeos {

namespace device_sync {

CryptAuthEnrollerFactoryImpl::CryptAuthEnrollerFactoryImpl(
    cryptauth::CryptAuthClientFactory* cryptauth_client_factory)
    : cryptauth_client_factory_(cryptauth_client_factory) {}

CryptAuthEnrollerFactoryImpl::~CryptAuthEnrollerFactoryImpl() = default;

std::unique_ptr<cryptauth::CryptAuthEnroller>
CryptAuthEnrollerFactoryImpl::CreateInstance() {
  return std::make_unique<cryptauth::CryptAuthEnrollerImpl>(
      cryptauth_client_factory_,
      cryptauth::SecureMessageDelegateImpl::Factory::NewInstance());
}

}  // namespace device_sync

}  // namespace chromeos
