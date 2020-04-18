// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/fake_authenticator.h"

#include <utility>


namespace cryptauth {

FakeAuthenticator::FakeAuthenticator() {}

FakeAuthenticator::~FakeAuthenticator() {}

void FakeAuthenticator::Authenticate(
    const Authenticator::AuthenticationCallback& callback) {
  last_callback_ = callback;
}

}  // namespace cryptauth
