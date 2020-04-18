// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_USERS_SUPERVISED_USER_MANAGER_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_USERS_SUPERVISED_USER_MANAGER_IMPL_H_

#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/login/supervised/supervised_user_authentication.h"
#include "chrome/browser/chromeos/login/users/supervised_user_manager.h"

namespace chromeos {

class ChromeUserManagerImpl;
class CrosSettings;
class SupervisedUserTestBase;

// Implementation of the UserManager.
class SupervisedUserManagerImpl : public SupervisedUserManager {
 public:
  ~SupervisedUserManagerImpl() override;

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
  void StartCreationTransaction(const base::string16& display_name) override;
  void SetCreationTransactionUserId(const std::string& user_id) override;
  void CommitCreationTransaction() override;
  SupervisedUserAuthentication* GetAuthentication() override;
  void GetPasswordInformation(const std::string& user_id,
                              base::DictionaryValue* result) override;
  void SetPasswordInformation(
      const std::string& user_id,
      const base::DictionaryValue* password_info) override;
  void LoadSupervisedUserToken(Profile* profile,
                               const LoadTokenCallback& callback) override;
  void ConfigureSyncWithToken(Profile* profile,
                              const std::string& token) override;

 private:
  friend class ChromeUserManagerImpl;
  friend class UserManager;
  friend class SupervisedUserTestBase;

  explicit SupervisedUserManagerImpl(ChromeUserManagerImpl* owner);

  // Returns true if there is non-committed user creation transaction.
  bool HasFailedUserCreationTransaction();

  // Attempts to clean up data that could be left from failed user creation.
  void RollbackUserCreationTransaction();

  void RemoveNonCryptohomeData(const std::string& user_id);

  bool CheckForFirstRun(const std::string& user_id);

  // Update name if this user is manager of some managed users.
  void UpdateManagerName(const std::string& manager_id,
                         const base::string16& new_display_name);

  bool GetUserStringValue(const std::string& user_id,
                          const char* key,
                          std::string* out_value) const;

  void SetUserStringValue(const std::string& user_id,
                          const char* key,
                          const std::string& value);

  bool GetUserIntegerValue(const std::string& user_id,
                           const char* key,
                           int* out_value) const;

  void SetUserIntegerValue(const std::string& user_id,
                           const char* key,
                           const int value);

  bool GetUserBooleanValue(const std::string& user_id,
                           const char* key,
                           bool* out_value) const;

  void SetUserBooleanValue(const std::string& user_id,
                           const char* key,
                           const bool value);

  void CleanPref(const std::string& user_id, const char* key);

  ChromeUserManagerImpl* owner_;

  // Interface to the signed settings store.
  CrosSettings* cros_settings_;

  std::unique_ptr<SupervisedUserAuthentication> authentication_;

  DISALLOW_COPY_AND_ASSIGN(SupervisedUserManagerImpl);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_USERS_SUPERVISED_USER_MANAGER_IMPL_H_
