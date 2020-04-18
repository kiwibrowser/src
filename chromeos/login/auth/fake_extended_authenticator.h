// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_LOGIN_AUTH_FAKE_EXTENDED_AUTHENTICATOR_H_
#define CHROMEOS_LOGIN_AUTH_FAKE_EXTENDED_AUTHENTICATOR_H_

#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/login/auth/extended_authenticator.h"
#include "chromeos/login/auth/user_context.h"

namespace chromeos {

class AuthFailure;

class CHROMEOS_EXPORT FakeExtendedAuthenticator : public ExtendedAuthenticator {
 public:
  FakeExtendedAuthenticator(NewAuthStatusConsumer* consumer,
                            const UserContext& expected_user_context);
  FakeExtendedAuthenticator(AuthStatusConsumer* consumer,
                            const UserContext& expected_user_context);

  // ExtendedAuthenticator:
  void SetConsumer(AuthStatusConsumer* consumer) override;
  void AuthenticateToMount(const UserContext& context,
                           const ResultCallback& success_callback) override;
  void AuthenticateToCheck(const UserContext& context,
                           const base::Closure& success_callback) override;
  void AddKey(const UserContext& context,
              const cryptohome::KeyDefinition& key,
              bool replace_existing,
              const base::Closure& success_callback) override;
  void UpdateKeyAuthorized(const UserContext& context,
                           const cryptohome::KeyDefinition& key,
                           const std::string& signature,
                           const base::Closure& success_callback) override;
  void RemoveKey(const UserContext& context,
                 const std::string& key_to_remove,
                 const base::Closure& success_callback) override;
  void TransformKeyIfNeeded(const UserContext& user_context,
                            const ContextCallback& callback) override;

 private:
  ~FakeExtendedAuthenticator() override;

  void OnAuthSuccess(const UserContext& context);
  void OnAuthFailure(AuthState state, const AuthFailure& error);

  NewAuthStatusConsumer* consumer_;
  AuthStatusConsumer* old_consumer_;

  UserContext expected_user_context_;

  DISALLOW_COPY_AND_ASSIGN(FakeExtendedAuthenticator);
};

}  // namespace chromeos

#endif  // CHROMEOS_LOGIN_AUTH_FAKE_EXTENDED_AUTHENTICATOR_H_
