// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_LOGIN_AUTH_STUB_AUTHENTICATOR_H_
#define CHROMEOS_LOGIN_AUTH_STUB_AUTHENTICATOR_H_

#include <string>

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/login/auth/authenticator.h"
#include "chromeos/login/auth/user_context.h"

class AccountId;

namespace content {
class BrowserContext;
}

namespace chromeos {

class AuthStatusConsumer;

class CHROMEOS_EXPORT StubAuthenticator : public Authenticator {
 public:
  StubAuthenticator(AuthStatusConsumer* consumer,
                    const UserContext& expected_user_context);

  // Authenticator:
  void CompleteLogin(content::BrowserContext* context,
                     const UserContext& user_context) override;
  void AuthenticateToLogin(content::BrowserContext* context,
                           const UserContext& user_context) override;
  void AuthenticateToUnlock(const UserContext& user_context) override;
  void LoginAsSupervisedUser(const UserContext& user_context) override;
  void LoginOffTheRecord() override;
  void LoginAsPublicSession(const UserContext& user_context) override;
  void LoginAsKioskAccount(const AccountId& app_account_id,
                           bool use_guest_mount) override;
  void LoginAsArcKioskAccount(const AccountId& app_account_id) override;
  void OnAuthSuccess() override;
  void OnAuthFailure(const AuthFailure& failure) override;
  void RecoverEncryptedData(const std::string& old_password) override;
  void ResyncEncryptedData() override;

  virtual void SetExpectedCredentials(const UserContext& user_context);

 protected:
  ~StubAuthenticator() override;

 private:
  UserContext expected_user_context_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(StubAuthenticator);
};

}  // namespace chromeos

#endif  // CHROMEOS_LOGIN_AUTH_STUB_AUTHENTICATOR_H_
