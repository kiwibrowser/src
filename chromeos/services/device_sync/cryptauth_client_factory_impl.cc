// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/device_sync/cryptauth_client_factory_impl.h"

#include "chromeos/services/device_sync/cryptauth_token_fetcher_impl.h"
#include "components/cryptauth/cryptauth_client_impl.h"

namespace chromeos {

namespace device_sync {

CryptAuthClientFactoryImpl::CryptAuthClientFactoryImpl(
    identity::IdentityManager* identity_manager,
    scoped_refptr<net::URLRequestContextGetter> url_request_context,
    const cryptauth::DeviceClassifier& device_classifier)
    : identity_manager_(identity_manager),
      url_request_context_(std::move(url_request_context)),
      device_classifier_(device_classifier) {}

CryptAuthClientFactoryImpl::~CryptAuthClientFactoryImpl() = default;

std::unique_ptr<cryptauth::CryptAuthClient>
CryptAuthClientFactoryImpl::CreateInstance() {
  return std::make_unique<cryptauth::CryptAuthClientImpl>(
      std::make_unique<cryptauth::CryptAuthApiCallFlow>(),
      std::make_unique<CryptAuthAccessTokenFetcherImpl>(identity_manager_),
      url_request_context_, device_classifier_);
}

}  // namespace device_sync

}  // namespace chromeos
