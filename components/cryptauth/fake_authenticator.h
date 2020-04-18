// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_FAKE_AUTHENTICATOR_H_
#define COMPONENTS_CRYPTAUTH_FAKE_AUTHENTICATOR_H_

#include "base/callback.h"
#include "base/macros.h"
#include "components/cryptauth/authenticator.h"

namespace cryptauth {

// A fake implementation of Authenticator to use in tests.
class FakeAuthenticator : public Authenticator {
 public:
  FakeAuthenticator();
  ~FakeAuthenticator() override;

  // Authenticator:
  void Authenticate(const AuthenticationCallback& callback)
      override;

  AuthenticationCallback last_callback() {
    return last_callback_;
  }

 private:
  AuthenticationCallback last_callback_;

  DISALLOW_COPY_AND_ASSIGN(FakeAuthenticator);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_FAKE_AUTHENTICATOR_H_
