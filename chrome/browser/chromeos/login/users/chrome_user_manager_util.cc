// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/users/chrome_user_manager_util.h"

#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/minimum_version_policy_handler.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_provider.h"
#include "chromeos/settings/cros_settings_names.h"
#include "components/prefs/pref_value_map.h"
#include "components/user_manager/user_manager.h"
#include "components/user_manager/user_names.h"
#include "components/user_manager/user_type.h"

namespace chromeos {
namespace chrome_user_manager_util {
namespace {
// Checks if constraint defined by minimum version policy is satisfied.
bool MinVersionConstraintsSatisfied() {
  return g_browser_process->platform_part()
      ->browser_policy_connector_chromeos()
      ->GetMinimumVersionPolicyHandler()
      ->RequirementsAreSatisfied();
}

bool IsUserAllowedInner(const user_manager::User& user,
                        bool supervised_users_allowed,
                        bool is_guest_allowed,
                        bool is_user_whitelisted) {
  if (user.GetType() == user_manager::USER_TYPE_GUEST && !is_guest_allowed)
    return false;
  if (user.GetType() == user_manager::USER_TYPE_SUPERVISED &&
      !supervised_users_allowed)
    return false;
  if (user.HasGaiaAccount() && !is_user_whitelisted)
    return false;
  if (!MinVersionConstraintsSatisfied() &&
      user.GetType() != user_manager::USER_TYPE_GUEST)
    return false;
  return true;
}
}  // namespace

bool GetPlatformKnownUserId(const std::string& user_email,
                            const std::string& gaia_id,
                            AccountId* out_account_id) {
  if (user_email == user_manager::kStubUserEmail) {
    *out_account_id = user_manager::StubAccountId();
    return true;
  }

  if (user_email == user_manager::kStubAdUserEmail) {
    *out_account_id = user_manager::StubAdAccountId();
    return true;
  }

  if (user_email == user_manager::kGuestUserName) {
    *out_account_id = user_manager::GuestAccountId();
    return true;
  }

  return false;
}

void UpdateLoginState(const user_manager::User* active_user,
                      const user_manager::User* primary_user,
                      bool is_current_user_owner) {
  if (!chromeos::LoginState::IsInitialized())
    return;  // LoginState may not be initialized in tests.

  chromeos::LoginState::LoggedInState logged_in_state;
  logged_in_state = active_user ? chromeos::LoginState::LOGGED_IN_ACTIVE
                                : chromeos::LoginState::LOGGED_IN_NONE;

  chromeos::LoginState::LoggedInUserType login_user_type;
  if (logged_in_state == chromeos::LoginState::LOGGED_IN_NONE)
    login_user_type = chromeos::LoginState::LOGGED_IN_USER_NONE;
  else if (is_current_user_owner)
    login_user_type = chromeos::LoginState::LOGGED_IN_USER_OWNER;
  else if (active_user->GetType() == user_manager::USER_TYPE_GUEST)
    login_user_type = chromeos::LoginState::LOGGED_IN_USER_GUEST;
  else if (active_user->GetType() == user_manager::USER_TYPE_PUBLIC_ACCOUNT)
    login_user_type = chromeos::LoginState::LOGGED_IN_USER_PUBLIC_ACCOUNT;
  else if (active_user->GetType() == user_manager::USER_TYPE_SUPERVISED)
    login_user_type = chromeos::LoginState::LOGGED_IN_USER_SUPERVISED;
  else if (active_user->GetType() == user_manager::USER_TYPE_KIOSK_APP)
    login_user_type = chromeos::LoginState::LOGGED_IN_USER_KIOSK_APP;
  else if (active_user->GetType() == user_manager::USER_TYPE_ARC_KIOSK_APP)
    login_user_type = chromeos::LoginState::LOGGED_IN_USER_ARC_KIOSK_APP;
  else
    login_user_type = chromeos::LoginState::LOGGED_IN_USER_REGULAR;

  if (primary_user) {
    chromeos::LoginState::Get()->SetLoggedInStateAndPrimaryUser(
        logged_in_state, login_user_type, primary_user->username_hash());
  } else {
    chromeos::LoginState::Get()->SetLoggedInState(logged_in_state,
                                                  login_user_type);
  }
}

bool AreSupervisedUsersAllowed(const CrosSettings* cros_settings) {
  bool supervised_users_allowed = false;
  cros_settings->GetBoolean(kAccountsPrefSupervisedUsersEnabled,
                            &supervised_users_allowed);
  return supervised_users_allowed;
}

bool IsGuestSessionAllowed(const CrosSettings* cros_settings) {
  const AccountId& owner_account_id =
      user_manager::UserManager::Get()->GetOwnerAccountId();
  if (owner_account_id.is_valid() &&
      user_manager::UserManager::Get()->FindUser(owner_account_id)->GetType() ==
          user_manager::UserType::USER_TYPE_CHILD) {
    return false;
  }

  // In tests CrosSettings might not be initialized.
  if (!cros_settings)
    return false;

  bool is_guest_allowed = false;
  cros_settings->GetBoolean(kAccountsPrefAllowGuest, &is_guest_allowed);
  return is_guest_allowed;
}

bool IsGaiaUserAllowed(const user_manager::User& user,
                       const CrosSettings* cros_settings) {
  DCHECK(user.HasGaiaAccount());
  return cros_settings->IsUserWhitelisted(user.GetAccountId().GetUserEmail(),
                                          nullptr);
}

bool IsUserAllowed(const user_manager::User& user,
                   const enterprise_management::ChromeDeviceSettingsProto&
                       device_settings_proto) {
  DCHECK(user.GetType() == user_manager::USER_TYPE_REGULAR ||
         user.GetType() == user_manager::USER_TYPE_GUEST ||
         user.GetType() == user_manager::USER_TYPE_SUPERVISED ||
         user.GetType() == user_manager::USER_TYPE_CHILD);

  PrefValueMap prefs;
  DeviceSettingsProvider::DecodePolicies(device_settings_proto, &prefs);

  bool supervised_users_allowed = false;
  prefs.GetBoolean(kAccountsPrefSupervisedUsersEnabled,
                   &supervised_users_allowed);

  bool is_guest_allowed = false;
  prefs.GetBoolean(kAccountsPrefAllowGuest, &is_guest_allowed);

  const base::Value* value;
  const base::ListValue* list;
  if (prefs.GetValue(kAccountsPrefUsers, &value)) {
    value->GetAsList(&list);
  }

  bool is_user_whitelisted =
      user.HasGaiaAccount() &&
      CrosSettings::FindEmailInList(list, user.GetAccountId().GetUserEmail(),
                                    nullptr);
  bool allow_new_user = false;
  prefs.GetBoolean(kAccountsPrefAllowNewUser, &allow_new_user);

  return IsUserAllowedInner(
      user, supervised_users_allowed, is_guest_allowed,
      user.HasGaiaAccount() && (allow_new_user || is_user_whitelisted));
}

bool IsUserAllowed(const user_manager::User& user,
                   const CrosSettings* cros_settings) {
  DCHECK(user.GetType() == user_manager::USER_TYPE_REGULAR ||
         user.GetType() == user_manager::USER_TYPE_GUEST ||
         user.GetType() == user_manager::USER_TYPE_SUPERVISED ||
         user.GetType() == user_manager::USER_TYPE_CHILD);

  return IsUserAllowedInner(
      user, AreSupervisedUsersAllowed(cros_settings),
      IsGuestSessionAllowed(cros_settings),
      user.HasGaiaAccount() && IsGaiaUserAllowed(user, cros_settings));
}

}  // namespace chrome_user_manager_util
}  // namespace chromeos
