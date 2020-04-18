// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_USERS_CHROME_USER_MANAGER_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_USERS_CHROME_USER_MANAGER_UTIL_H_

#include "chromeos/login/login_state.h"
#include "components/policy/proto/chrome_device_policy.pb.h"
#include "components/user_manager/user.h"

class AccountId;
class CrosSettings;

namespace chromeos {
namespace chrome_user_manager_util {

// Implements user_manager::UserManager::GetPlatformKnownUserId for ChromeOS
bool GetPlatformKnownUserId(const std::string& user_email,
                            const std::string& gaia_id,
                            AccountId* out_account_id);

// Implements user_manager::UserManager::UpdateLoginState.
void UpdateLoginState(const user_manager::User* active_user,
                      const user_manager::User* primary_user,
                      bool is_current_user_owner);

// Check if supervised users are allowed by provided cros settings.
bool AreSupervisedUsersAllowed(const CrosSettings* cros_settings);

// Check if guest user is allowed by provided cros settings.
bool IsGuestSessionAllowed(const CrosSettings* cros_settings);

// Check if the provided gaia user is allowed by provided cros settings.
bool IsGaiaUserAllowed(const user_manager::User& user,
                       const CrosSettings* cros_settings);

// Returns true if |user| is allowed depending on provided device policies.
// Accepted user types: USER_TYPE_REGULAR, USER_TYPE_GUEST,
// USER_TYPE_SUPERVISED, USER_TYPE_CHILD.
bool IsUserAllowed(const user_manager::User& user,
                   const enterprise_management::ChromeDeviceSettingsProto&
                       device_settings_proto);

// Returns true if |user| is allowed depending on provided cros settings.
// Accepted user types: USER_TYPE_REGULAR, USER_TYPE_GUEST,
// USER_TYPE_SUPERVISED, USER_TYPE_CHILD.
bool IsUserAllowed(const user_manager::User& user,
                   const CrosSettings* cros_settings);

}  // namespace chrome_user_manager_util
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_USERS_CHROME_USER_MANAGER_UTIL_H_
