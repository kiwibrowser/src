// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/oauth2_access_token_fetcher.h"

OAuth2AccessTokenFetcher::OAuth2AccessTokenFetcher(
    OAuth2AccessTokenConsumer* consumer)
    : consumer_(consumer) {}

OAuth2AccessTokenFetcher::~OAuth2AccessTokenFetcher() {}

void OAuth2AccessTokenFetcher::FireOnGetTokenSuccess(
    const std::string& access_token,
    const base::Time& expiration_time) {
  consumer_->OnGetTokenSuccess(access_token, expiration_time);
}

void OAuth2AccessTokenFetcher::FireOnGetTokenFailure(
    const GoogleServiceAuthError& error) {
  consumer_->OnGetTokenFailure(error);
}
