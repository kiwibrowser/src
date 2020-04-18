// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_DEVICE_SYNC_CRYPTAUTH_TOKEN_FETCHER_IMPL_H_
#define CHROMEOS_SERVICES_DEVICE_SYNC_CRYPTAUTH_TOKEN_FETCHER_IMPL_H_

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/cryptauth/cryptauth_access_token_fetcher.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace identity {
class IdentityManager;
class PrimaryAccountAccessTokenFetcher;
}  // namespace identity

namespace chromeos {

namespace device_sync {

// CryptAuthAccessTokenFetcher implementation which utilizes IdentityManager.
class CryptAuthAccessTokenFetcherImpl
    : public cryptauth::CryptAuthAccessTokenFetcher {
 public:
  CryptAuthAccessTokenFetcherImpl(identity::IdentityManager* identity_manager);

  ~CryptAuthAccessTokenFetcherImpl() override;

  // cryptauth::CryptAuthAccessTokenFetcher:
  void FetchAccessToken(const AccessTokenCallback& callback) override;

 private:
  void InvokeThenClearPendingCallbacks(const std::string& access_token);
  void OnAccessTokenFetched(const GoogleServiceAuthError& error,
                            const std::string& access_token);

  identity::IdentityManager* identity_manager_;

  std::unique_ptr<identity::PrimaryAccountAccessTokenFetcher>
      access_token_fetcher_;
  std::vector<AccessTokenCallback> pending_callbacks_;

  base::WeakPtrFactory<CryptAuthAccessTokenFetcherImpl> weak_ptr_factory_;
};

}  // namespace device_sync

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_DEVICE_SYNC_CRYPTAUTH_TOKEN_FETCHER_IMPL_H_
