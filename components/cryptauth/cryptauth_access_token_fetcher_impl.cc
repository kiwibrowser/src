// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/cryptauth_access_token_fetcher_impl.h"

namespace cryptauth {

namespace {

// Returns the set of OAuth2 scopes that CryptAuth uses.
OAuth2TokenService::ScopeSet GetScopes() {
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert("https://www.googleapis.com/auth/cryptauth");
  return scopes;
}

}  // namespace

CryptAuthAccessTokenFetcherImpl::CryptAuthAccessTokenFetcherImpl(
    OAuth2TokenService* token_service,
    const std::string& account_id)
    : OAuth2TokenService::Consumer("cryptauth_access_token_fetcher"),
      token_service_(token_service),
      account_id_(account_id),
      fetch_started_(false) {
}

CryptAuthAccessTokenFetcherImpl::~CryptAuthAccessTokenFetcherImpl() {
}

void CryptAuthAccessTokenFetcherImpl::FetchAccessToken(
    const AccessTokenCallback& callback) {
  if (fetch_started_) {
    LOG(WARNING) << "Create an instance for each token fetched. Do not reuse.";
    callback.Run(std::string());
    return;
  }

  fetch_started_ = true;
  callback_ = callback;
  // This request will return a cached result if it is available, saving a
  // network round trip every time we fetch the access token.
  token_request_ = token_service_->StartRequest(account_id_, GetScopes(), this);
}

void CryptAuthAccessTokenFetcherImpl::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  callback_.Run(access_token);
}

void CryptAuthAccessTokenFetcherImpl::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  callback_.Run(std::string());
}

}  // namespace cryptauth
