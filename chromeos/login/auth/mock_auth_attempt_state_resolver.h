// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_LOGIN_AUTH_MOCK_AUTH_ATTEMPT_STATE_RESOLVER_H_
#define CHROMEOS_LOGIN_AUTH_MOCK_AUTH_ATTEMPT_STATE_RESOLVER_H_

#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/login/auth/auth_attempt_state_resolver.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {

class CHROMEOS_EXPORT MockAuthAttemptStateResolver
    : public AuthAttemptStateResolver {
 public:
  MockAuthAttemptStateResolver();
  virtual ~MockAuthAttemptStateResolver();

  MOCK_METHOD0(Resolve, void(void));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockAuthAttemptStateResolver);
};

}  // namespace chromeos

#endif  // CHROMEOS_LOGIN_AUTH_MOCK_AUTH_ATTEMPT_STATE_RESOLVER_H_
