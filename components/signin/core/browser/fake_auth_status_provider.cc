// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/fake_auth_status_provider.h"

FakeAuthStatusProvider::FakeAuthStatusProvider(SigninErrorController* error)
  : error_provider_(error),
    auth_error_(GoogleServiceAuthError::AuthErrorNone()) {
  error_provider_->AddProvider(this);
}

FakeAuthStatusProvider::~FakeAuthStatusProvider() {
  error_provider_->RemoveProvider(this);
}

std::string FakeAuthStatusProvider::GetAccountId() const {
  return account_id_;
}

GoogleServiceAuthError FakeAuthStatusProvider::GetAuthStatus() const {
  return auth_error_;
}

void FakeAuthStatusProvider::SetAuthError(const std::string& account_id,
                                          const GoogleServiceAuthError& error) {
  account_id_ = account_id;
  auth_error_ = error;
  error_provider_->AuthStatusChanged();
}
