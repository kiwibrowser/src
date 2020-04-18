// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/device_sync/cryptauth_token_fetcher_impl.h"

#include <set>

#include "services/identity/public/cpp/identity_manager.h"

namespace chromeos {

namespace device_sync {

namespace {

const char kIdentityManagerConsumerName[] = "multi_device_service";
const char kCryptAuthApiScope[] = "https://www.googleapis.com/auth/cryptauth";

}  // namespace

CryptAuthAccessTokenFetcherImpl::CryptAuthAccessTokenFetcherImpl(
    identity::IdentityManager* identity_manager)
    : identity_manager_(identity_manager), weak_ptr_factory_(this) {}

CryptAuthAccessTokenFetcherImpl::~CryptAuthAccessTokenFetcherImpl() {
  // If any callbacks are still pending when the object is deleted, run them
  // now, passing an empty string to indicate failure. This ensures that no
  // callbacks are left hanging forever.
  InvokeThenClearPendingCallbacks(std::string());
}

void CryptAuthAccessTokenFetcherImpl::FetchAccessToken(
    const AccessTokenCallback& callback) {
  pending_callbacks_.emplace_back(callback);

  // If the token is already being fetched, continue waiting for it.
  if (access_token_fetcher_)
    return;

  const OAuth2TokenService::ScopeSet kScopes{kCryptAuthApiScope};
  access_token_fetcher_ =
      identity_manager_->CreateAccessTokenFetcherForPrimaryAccount(
          kIdentityManagerConsumerName, kScopes,
          base::BindOnce(&CryptAuthAccessTokenFetcherImpl::OnAccessTokenFetched,
                         weak_ptr_factory_.GetWeakPtr()),
          identity::PrimaryAccountAccessTokenFetcher::Mode::kImmediate);
}

void CryptAuthAccessTokenFetcherImpl::InvokeThenClearPendingCallbacks(
    const std::string& access_token) {
  for (auto& callback : pending_callbacks_)
    callback.Run(access_token);

  pending_callbacks_.clear();
}

void CryptAuthAccessTokenFetcherImpl::OnAccessTokenFetched(
    const GoogleServiceAuthError& error,
    const std::string& access_token) {
  // Move |access_token_fetcher_| to a temporary std::unique_ptr so that it is
  // deleted at the end of this function. This is done instead of deleting
  // |access_token_fetcher_| at the end of this function to ensure that if any
  // observers invoke FetchAccessToken(), a new object will be created.
  std::unique_ptr<identity::PrimaryAccountAccessTokenFetcher>
      token_fetcher_deleter = std::move(access_token_fetcher_);

  InvokeThenClearPendingCallbacks(
      error == GoogleServiceAuthError::AuthErrorNone() ? access_token
                                                       : std::string());
}

}  // namespace device_sync

}  // namespace chromeos
