// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_CRYPTAUTH_ACCESS_TOKEN_FETCHER_IMPL_H_
#define COMPONENTS_CRYPTAUTH_CRYPTAUTH_ACCESS_TOKEN_FETCHER_IMPL_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "components/cryptauth/cryptauth_access_token_fetcher.h"
#include "google_apis/gaia/oauth2_token_service.h"

namespace cryptauth {

// Implementation of CryptAuthAccessTokenFetcher fetching an access token for a
// given account using the provided OAuth2TokenService.
class CryptAuthAccessTokenFetcherImpl : public CryptAuthAccessTokenFetcher,
                                        public OAuth2TokenService::Consumer {
 public:
  // |token_service| is not owned, and must outlive this object.
  CryptAuthAccessTokenFetcherImpl(OAuth2TokenService* token_service,
                                  const std::string& account_id);
  ~CryptAuthAccessTokenFetcherImpl() override;

  // CryptAuthAccessTokenFetcher:
  void FetchAccessToken(const AccessTokenCallback& callback) override;

 private:
  // OAuth2TokenService::Consumer:
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  // System service that caches and fetches tokens for a given account.
  // Not owned.
  OAuth2TokenService* token_service_;

  // The account id for whom to mint the token.
  std::string account_id_;

  // True if FetchAccessToken() has been called.
  bool fetch_started_;

  // Stores the request from |token_service_| to mint the token.
  std::unique_ptr<OAuth2TokenService::Request> token_request_;

  // Callback to invoke when the token fetch succeeds or fails.
  AccessTokenCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(CryptAuthAccessTokenFetcherImpl);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_CRYPTAUTH_ACCESS_TOKEN_FETCHER_IMPL_H_
