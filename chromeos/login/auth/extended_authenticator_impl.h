// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_LOGIN_AUTH_EXTENDED_AUTHENTICATOR_IMPL_H_
#define CHROMEOS_LOGIN_AUTH_EXTENDED_AUTHENTICATOR_IMPL_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/optional.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/cryptohome/rpc.pb.h"
#include "chromeos/login/auth/extended_authenticator.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

class AuthStatusConsumer;
class UserContext;

// Implements ExtendedAuthenticator.
class CHROMEOS_EXPORT ExtendedAuthenticatorImpl : public ExtendedAuthenticator {
 public:
  explicit ExtendedAuthenticatorImpl(NewAuthStatusConsumer* consumer);
  explicit ExtendedAuthenticatorImpl(AuthStatusConsumer* consumer);

  // ExtendedAuthenticator:
  void SetConsumer(AuthStatusConsumer* consumer) override;
  void AuthenticateToMount(const UserContext& context,
                           const ResultCallback& success_callback) override;
  void AuthenticateToCheck(const UserContext& context,
                           const base::Closure& success_callback) override;
  void AddKey(const UserContext& context,
              const cryptohome::KeyDefinition& key,
              bool clobber_if_exists,
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
  ~ExtendedAuthenticatorImpl() override;

  // Callback for system salt getter.
  void OnSaltObtained(const std::string& system_salt);

  // Performs actual operation with fully configured |context|.
  void DoAuthenticateToMount(const ResultCallback& success_callback,
                             const UserContext& context);
  void DoAuthenticateToCheck(const base::Closure& success_callback,
                             const UserContext& context);
  void DoAddKey(const cryptohome::KeyDefinition& key,
                bool clobber_if_exists,
                const base::Closure& success_callback,
                const UserContext& context);
  void DoUpdateKeyAuthorized(const cryptohome::KeyDefinition& key,
                             const std::string& signature,
                             const base::Closure& success_callback,
                             const UserContext& context);
  void DoRemoveKey(const std::string& key_to_remove,
                   const base::Closure& success_callback,
                   const UserContext& context);

  // Inner operation callbacks.
  void OnMountComplete(const std::string& time_marker,
                       const UserContext& context,
                       const ResultCallback& success_callback,
                       base::Optional<cryptohome::BaseReply> reply);
  void OnOperationComplete(const std::string& time_marker,
                           const UserContext& context,
                           const base::Closure& success_callback,
                           bool success,
                           cryptohome::MountError return_code);

  bool salt_obtained_;
  std::string system_salt_;
  std::vector<base::Closure> system_salt_callbacks_;

  NewAuthStatusConsumer* consumer_;
  AuthStatusConsumer* old_consumer_;

  DISALLOW_COPY_AND_ASSIGN(ExtendedAuthenticatorImpl);
};

}  // namespace chromeos

#endif  // CHROMEOS_LOGIN_AUTH_EXTENDED_AUTHENTICATOR_IMPL_H_
