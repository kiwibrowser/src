// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/signin_manager_wrapper.h"

#include "services/identity/public/cpp/identity_manager.h"

SigninManagerWrapper::SigninManagerWrapper(
    identity::IdentityManager* identity_manager,
    SigninManagerBase* signin_manager)
    : identity_manager_(identity_manager), signin_manager_(signin_manager) {}

SigninManagerWrapper::~SigninManagerWrapper() {}

identity::IdentityManager* SigninManagerWrapper::GetIdentityManager() {
  return identity_manager_;
}

SigninManagerBase* SigninManagerWrapper::GetSigninManager() {
  return signin_manager_;
}
