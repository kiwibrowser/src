// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_USERS_SUPERVISED_USER_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_USERS_SUPERVISED_USER_MANAGER_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"

class PrefRegistrySimple;

namespace user_manager {
class User;
}

namespace chromeos {

class SupervisedUserAuthentication;

// Keys in dictionary with supervised password information.
extern const char kSchemaVersion[];
extern const char kPasswordRevision[];
extern const char kSalt[];
extern const char kRequirePasswordUpdate[];
extern const char kHasIncompleteKey[];
extern const int kMinPasswordRevision;

// Values for these keys are not stored in local state.
extern const char kEncryptedPassword[];
extern const char kPasswordSignature[];
extern const char kPasswordEncryptionKey[];
extern const char kPasswordSignatureKey[];

extern const char kPasswordUpdateFile[];

// Base class for SupervisedUserManagerImpl - provides a mechanism for getting
// and setting specific values for supervised users, as well as additional
// lookup methods that make sense only for supervised users.
class SupervisedUserManager {
 public:
  typedef base::Callback<void(const std::string& /* token */)>
      LoadTokenCallback;

  // Registers user manager preferences.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  SupervisedUserManager() {}
  virtual ~SupervisedUserManager() {}

  // Checks if given user have supervised users on this device.

  virtual bool HasSupervisedUsers(const std::string& manager_id) const = 0;

  // Creates supervised user with given |display_name| and |local_user_id|
  // and persists that to user list. Also links this user identified by
  // |sync_user_id| to manager with a |manager_id|.
  // Returns created user, or existing user if there already
  // was a supervised user with such display name.
  // TODO(antrim): Refactor into a single struct to have only 1 getter.
  virtual const user_manager::User* CreateUserRecord(
      const std::string& manager_id,
      const std::string& local_user_id,
      const std::string& sync_user_id,
      const base::string16& display_name) = 0;

  // Generates unique user ID for supervised user.
  virtual std::string GenerateUserId() = 0;

  // Returns the supervised user with the given |display_name| if found in
  // the persistent list. Returns |NULL| otherwise.
  virtual const user_manager::User* FindByDisplayName(
      const base::string16& display_name) const = 0;

  // Returns the supervised user with the given |sync_id| if found in
  // the persistent list. Returns |NULL| otherwise.
  virtual const user_manager::User* FindBySyncId(
      const std::string& sync_id) const = 0;

  // Returns sync_user_id for supervised user with |user_id| or empty string if
  // such user is not found or it doesn't have user_id defined.
  virtual std::string GetUserSyncId(const std::string& user_id) const = 0;

  // Returns the display name for manager of user |user_id| if it is known
  // (was previously set by a |SaveUserDisplayName| call).
  // Otherwise, returns a manager id.
  virtual base::string16 GetManagerDisplayName(
      const std::string& user_id) const = 0;

  // Returns the user id for manager of user |user_id| if it is known (user is
  // actually a managed user).
  // Otherwise, returns an empty string.
  virtual std::string GetManagerUserId(const std::string& user_id) const = 0;

  // Returns the display email for manager of user |user_id| if it is known
  // (user is actually a managed user).
  // Otherwise, returns an empty string.
  virtual std::string GetManagerDisplayEmail(
      const std::string& user_id) const = 0;

  // Create a record about starting supervised user creation transaction.
  virtual void StartCreationTransaction(const base::string16& display_name) = 0;

  // Add user id to supervised user creation transaction record.
  virtual void SetCreationTransactionUserId(const std::string& user_id) = 0;

  // Remove supervised user creation transaction record.
  virtual void CommitCreationTransaction() = 0;

  // Return object that handles specifics of supervised user authentication.
  virtual SupervisedUserAuthentication* GetAuthentication() = 0;

  // Fill |result| with public password-specific data for |user_id| from Local
  // State.
  virtual void GetPasswordInformation(const std::string& user_id,
                                      base::DictionaryValue* result) = 0;

  // Stores public password-specific data from |password_info| for |user_id| in
  // Local State.
  virtual void SetPasswordInformation(
      const std::string& user_id,
      const base::DictionaryValue* password_info) = 0;

  // Loads a sync oauth token in background, and passes it to callback.
  virtual void LoadSupervisedUserToken(Profile* profile,
                                       const LoadTokenCallback& callback) = 0;

  // Configures sync service with oauth token.
  virtual void ConfigureSyncWithToken(Profile* profile,
                                      const std::string& token) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(SupervisedUserManager);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_USERS_SUPERVISED_USER_MANAGER_H_
