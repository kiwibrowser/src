// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/identity_provider.h"

IdentityProvider::Observer::~Observer() {}

IdentityProvider::~IdentityProvider() {}

void IdentityProvider::AddActiveAccountRefreshTokenObserver(
    OAuth2TokenService::Observer* observer) {
  OAuth2TokenService* token_service = GetTokenService();
  if (!token_service || token_service_observers_.HasObserver(observer))
    return;

  token_service_observers_.AddObserver(observer);
  if (++token_service_observer_count_ == 1)
    token_service->AddObserver(this);
}

void IdentityProvider::RemoveActiveAccountRefreshTokenObserver(
    OAuth2TokenService::Observer* observer) {
  OAuth2TokenService* token_service = GetTokenService();
  if (!token_service || !token_service_observers_.HasObserver(observer))
    return;

  token_service_observers_.RemoveObserver(observer);
  if (--token_service_observer_count_ == 0)
    token_service->RemoveObserver(this);
}

void IdentityProvider::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void IdentityProvider::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void IdentityProvider::OnRefreshTokenAvailable(const std::string& account_id) {
  if (account_id != GetActiveAccountId())
    return;
  for (auto& observer : token_service_observers_)
    observer.OnRefreshTokenAvailable(account_id);
}

void IdentityProvider::OnRefreshTokenRevoked(const std::string& account_id) {
  if (account_id != GetActiveAccountId())
    return;
  for (auto& observer : token_service_observers_)
    observer.OnRefreshTokenRevoked(account_id);
}

void IdentityProvider::OnRefreshTokensLoaded() {
  for (auto& observer : token_service_observers_)
    observer.OnRefreshTokensLoaded();
}

IdentityProvider::IdentityProvider() : token_service_observer_count_(0) {}

void IdentityProvider::FireOnActiveAccountLogin() {
  for (auto& observer : observers_)
    observer.OnActiveAccountLogin();
}

void IdentityProvider::FireOnActiveAccountLogout() {
  for (auto& observer : observers_)
    observer.OnActiveAccountLogout();
}
