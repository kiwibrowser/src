// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/fake_identity_provider.h"

#include "google_apis/gaia/oauth2_token_service.h"

FakeIdentityProvider::FakeIdentityProvider(OAuth2TokenService* token_service)
    : token_service_(token_service) {
}

FakeIdentityProvider::~FakeIdentityProvider() {
}

void FakeIdentityProvider::SetActiveUsername(const std::string& account_id) {
  account_id_ = account_id;
}

void FakeIdentityProvider::LogIn(const std::string& account_id) {
  SetActiveUsername(account_id);
  FireOnActiveAccountLogin();
}

void FakeIdentityProvider::LogOut() {
  account_id_.clear();
  FireOnActiveAccountLogout();
}

std::string FakeIdentityProvider::GetActiveUsername() {
  return account_id_;
}

std::string FakeIdentityProvider::GetActiveAccountId() {
  return account_id_;
}

OAuth2TokenService* FakeIdentityProvider::GetTokenService() {
  return token_service_;
}
