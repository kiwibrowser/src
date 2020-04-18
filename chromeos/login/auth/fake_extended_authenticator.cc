// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/login/auth/fake_extended_authenticator.h"

#include "base/logging.h"
#include "chromeos/login/auth/auth_status_consumer.h"
#include "components/account_id/account_id.h"

namespace chromeos {

FakeExtendedAuthenticator::FakeExtendedAuthenticator(
    NewAuthStatusConsumer* consumer,
    const UserContext& expected_user_context)
    : consumer_(consumer),
      old_consumer_(NULL),
      expected_user_context_(expected_user_context) {
}

FakeExtendedAuthenticator::FakeExtendedAuthenticator(
    AuthStatusConsumer* consumer,
    const UserContext& expected_user_context)
    : consumer_(NULL),
      old_consumer_(consumer),
      expected_user_context_(expected_user_context) {
}

FakeExtendedAuthenticator::~FakeExtendedAuthenticator() = default;

void FakeExtendedAuthenticator::SetConsumer(AuthStatusConsumer* consumer) {
  old_consumer_ = consumer;
}

void FakeExtendedAuthenticator::AuthenticateToMount(
    const UserContext& context,
    const ResultCallback& success_callback) {
  if (expected_user_context_ == context) {
    UserContext reported_user_context(context);
    const std::string mount_hash =
        reported_user_context.GetAccountId().GetUserEmail() + "-hash";
    reported_user_context.SetUserIDHash(mount_hash);
    if (!success_callback.is_null())
      success_callback.Run(mount_hash);
    OnAuthSuccess(reported_user_context);
    return;
  }

  OnAuthFailure(FAILED_MOUNT,
                AuthFailure(AuthFailure::COULD_NOT_MOUNT_CRYPTOHOME));
}

void FakeExtendedAuthenticator::AuthenticateToCheck(
    const UserContext& context,
    const base::Closure& success_callback) {
  if (expected_user_context_ == context) {
    if (!success_callback.is_null())
      success_callback.Run();
    OnAuthSuccess(context);
    return;
  }

  OnAuthFailure(FAILED_MOUNT,
                AuthFailure(AuthFailure::UNLOCK_FAILED));
}

void FakeExtendedAuthenticator::AddKey(const UserContext& context,
                    const cryptohome::KeyDefinition& key,
                    bool replace_existing,
                    const base::Closure& success_callback) {
  NOTREACHED();
}

void FakeExtendedAuthenticator::UpdateKeyAuthorized(
    const UserContext& context,
    const cryptohome::KeyDefinition& key,
    const std::string& signature,
    const base::Closure& success_callback) {
  NOTREACHED();
}

void FakeExtendedAuthenticator::RemoveKey(const UserContext& context,
                       const std::string& key_to_remove,
                       const base::Closure& success_callback) {
  NOTREACHED();
}

void FakeExtendedAuthenticator::TransformKeyIfNeeded(
    const UserContext& user_context,
    const ContextCallback& callback) {
  if (!callback.is_null())
    callback.Run(user_context);
}

void FakeExtendedAuthenticator::OnAuthSuccess(const UserContext& context) {
  if (old_consumer_)
    old_consumer_->OnAuthSuccess(context);
}

void FakeExtendedAuthenticator::OnAuthFailure(AuthState state,
                                              const AuthFailure& error) {
  if (consumer_)
    consumer_->OnAuthenticationFailure(state);
  if (old_consumer_)
    old_consumer_->OnAuthFailure(error);
}

}  // namespace chromeos
