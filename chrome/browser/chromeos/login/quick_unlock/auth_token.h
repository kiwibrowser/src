// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_QUICK_UNLOCK_AUTH_TOKEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_QUICK_UNLOCK_AUTH_TOKEN_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/unguessable_token.h"

namespace chromeos {
class UserContext;

namespace quick_unlock {

class AuthToken {
 public:
  // How long the token lives.
  static const int kTokenExpirationSeconds;

  explicit AuthToken(const chromeos::UserContext& user_context);
  ~AuthToken();

  // An unguessable identifier that can be passed to webui to verify the token
  // instance has not changed. Returns nullopt if the token is expired.
  base::Optional<std::string> Identifier();

  // The UserContext returned here can be null if its time-to-live has expired.
  chromeos::UserContext* user_context() { return user_context_.get(); }

  // Time the token out - should be used only by tests.
  void ResetForTest();

 private:
  // Times the token out.
  void Reset();

  base::UnguessableToken identifier_;
  std::unique_ptr<chromeos::UserContext> user_context_;

  base::WeakPtrFactory<AuthToken> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AuthToken);
};

}  // namespace quick_unlock
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_QUICK_UNLOCK_AUTH_TOKEN_H_
