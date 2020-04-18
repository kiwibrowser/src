// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/one_google_bar/one_google_bar_service.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_loader.h"
#include "components/signin/core/browser/gaia_cookie_manager_service.h"

class OneGoogleBarService::SigninObserver
    : public GaiaCookieManagerService::Observer {
 public:
  using SigninStatusChangedCallback = base::Closure;

  SigninObserver(GaiaCookieManagerService* cookie_service,
                 const SigninStatusChangedCallback& callback)
      : cookie_service_(cookie_service), callback_(callback) {
    cookie_service_->AddObserver(this);
  }

  ~SigninObserver() override { cookie_service_->RemoveObserver(this); }

 private:
  // GaiaCookieManagerService::Observer implementation.
  void OnGaiaAccountsInCookieUpdated(const std::vector<gaia::ListedAccount>&,
                                     const std::vector<gaia::ListedAccount>&,
                                     const GoogleServiceAuthError&) override {
    callback_.Run();
  }

  GaiaCookieManagerService* const cookie_service_;
  SigninStatusChangedCallback callback_;
};

OneGoogleBarService::OneGoogleBarService(
    GaiaCookieManagerService* cookie_service,
    std::unique_ptr<OneGoogleBarLoader> loader)
    : loader_(std::move(loader)),
      signin_observer_(std::make_unique<SigninObserver>(
          cookie_service,
          base::Bind(&OneGoogleBarService::SigninStatusChanged,
                     base::Unretained(this)))) {}

OneGoogleBarService::~OneGoogleBarService() = default;

void OneGoogleBarService::Shutdown() {
  for (auto& observer : observers_) {
    observer.OnOneGoogleBarServiceShuttingDown();
  }

  signin_observer_.reset();
}

void OneGoogleBarService::Refresh() {
  loader_->Load(base::BindOnce(&OneGoogleBarService::OneGoogleBarDataLoaded,
                               base::Unretained(this)));
}

void OneGoogleBarService::AddObserver(OneGoogleBarServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void OneGoogleBarService::RemoveObserver(
    OneGoogleBarServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void OneGoogleBarService::SigninStatusChanged() {
  // If we have cached data, clear it and notify observers.
  if (one_google_bar_data_.has_value()) {
    one_google_bar_data_ = base::nullopt;
    NotifyObservers();
  }
}

void OneGoogleBarService::OneGoogleBarDataLoaded(
    OneGoogleBarLoader::Status status,
    const base::Optional<OneGoogleBarData>& data) {
  // In case of transient errors, keep our cached data (if any), but still
  // notify observers of the finished load (attempt).
  if (status != OneGoogleBarLoader::Status::TRANSIENT_ERROR) {
    one_google_bar_data_ = data;
  }
  NotifyObservers();
}

void OneGoogleBarService::NotifyObservers() {
  for (auto& observer : observers_) {
    observer.OnOneGoogleBarDataUpdated();
  }
}
