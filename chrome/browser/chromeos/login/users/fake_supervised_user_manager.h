// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_USERS_FAKE_SUPERVISED_USER_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_USERS_FAKE_SUPERVISED_USER_MANAGER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/login/users/supervised_user_manager.h"

namespace chromeos {

// Fake supervised user manager with a barebones implementation.
class FakeSupervisedUserManager : public SupervisedUserManager {
 public:
  FakeSupervisedUserManager();
  ~FakeSupervisedUserManager() override;

  bool HasSupervisedUsers(const std::string& manager_id) const override;
  const user_manager::User* CreateUserRecord(
      const std::string& manager_id,
      const std::string& local_user_id,
      const std::string& sync_user_id,
      const base::string16& display_name) override;
  std::string GenerateUserId() override;
  const user_manager::User* FindByDisplayName(
      const base::string16& display_name) const override;
  const user_manager::User* FindBySyncId(
      const std::string& sync_id) const override;
  std::string GetUserSyncId(const std::string& user_id) const override;
  base::string16 GetManagerDisplayName(
      const std::string& user_id) const override;
  std::string GetManagerUserId(const std::string& user_id) const override;
  std::string GetManagerDisplayEmail(const std::string& user_id) const override;
  void StartCreationTransaction(const base::string16& display_name) override {}
  void SetCreationTransactionUserId(const std::string& user_id) override {}
  void CommitCreationTransaction() override {}
  SupervisedUserAuthentication* GetAuthentication() override;
  void GetPasswordInformation(const std::string& user_id,
                              base::DictionaryValue* result) override {}
  void SetPasswordInformation(
      const std::string& user_id,
      const base::DictionaryValue* password_info) override {}
  void LoadSupervisedUserToken(Profile* profile,
                               const LoadTokenCallback& callback) override;
  void ConfigureSyncWithToken(Profile* profile,
                              const std::string& token) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeSupervisedUserManager);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_USERS_FAKE_SUPERVISED_USER_MANAGER_H_
