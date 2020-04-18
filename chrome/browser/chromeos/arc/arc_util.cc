// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/arc_util.h"

#include <linux/magic.h>
#include <sys/statfs.h>
#include <map>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chrome/browser/chromeos/arc/policy/arc_policy_util.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/login/user_flow.h"
#include "chrome/browser/chromeos/login/users/chrome_user_manager.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chromeos/chromeos_switches.h"
#include "components/arc/arc_features.h"
#include "components/arc/arc_prefs.h"
#include "components/arc/arc_util.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/known_user.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"

namespace arc {

namespace {

constexpr char kLsbReleaseArcVersionKey[] = "CHROMEOS_ARC_ANDROID_SDK_VERSION";
constexpr char kAndroidMSdkVersion[] = "23";

// Contains map of profile to check result of ARC allowed. Contains true if ARC
// allowed check was performed and ARC is allowed. If map does not contain
// a value then this means that check has not been performed yet.
base::LazyInstance<std::map<const Profile*, bool>>::DestructorAtExit
    g_profile_status_check = LAZY_INSTANCE_INITIALIZER;

// The cached value of migration allowed for profile. It is necessary to use
// the same value during a user session.
base::LazyInstance<std::map<base::FilePath, bool>>::DestructorAtExit
    g_is_arc_migration_allowed = LAZY_INSTANCE_INITIALIZER;

// Let IsAllowedForProfile() return "false" for any profile.
bool g_disallow_for_testing = false;

// Let IsArcBlockedDueToIncompatibleFileSystem() return the specified value
// during test runs. Doesn't affect ARC kiosk and public session.
bool g_arc_blocked_due_to_incompatible_filesystem_for_testing = false;

// TODO(kinaba): Temporary workaround for crbug.com/729034.
//
// Some type of accounts don't have user prefs. As a short-term workaround,
// store the compatibility info from them on memory, ignoring the defect that
// it cannot survive browser crash and restart.
//
// This will be removed once the forced migration for ARC Kiosk user is
// implemented. After it's done such types of accounts cannot even sign-in
// with incompatible filesystem. Hence it'll be safe to always regard compatible
// for them then.
base::LazyInstance<std::set<AccountId>>::DestructorAtExit
    g_known_compatible_users = LAZY_INSTANCE_INITIALIZER;

// Returns whether ARC can run on the filesystem mounted at |path|.
// This function should run only on threads where IO operations are allowed.
bool IsArcCompatibleFilesystem(const base::FilePath& path) {
  base::AssertBlockingAllowed();

  // If it can be verified it is not on ecryptfs, then it is ok.
  struct statfs statfs_buf;
  if (statfs(path.value().c_str(), &statfs_buf) < 0)
    return false;
  return statfs_buf.f_type != ECRYPTFS_SUPER_MAGIC;
}

FileSystemCompatibilityState GetFileSystemCompatibilityPref(
    const AccountId& account_id) {
  int pref_value = kFileSystemIncompatible;
  user_manager::known_user::GetIntegerPref(
      account_id, prefs::kArcCompatibleFilesystemChosen, &pref_value);
  return static_cast<FileSystemCompatibilityState>(pref_value);
}

// Stores the result of IsArcCompatibleFilesystem posted back from the blocking
// task runner.
void StoreCompatibilityCheckResult(const AccountId& account_id,
                                   const base::Closure& callback,
                                   bool is_compatible) {
  if (is_compatible) {
    user_manager::known_user::SetIntegerPref(
        account_id, prefs::kArcCompatibleFilesystemChosen,
        kFileSystemCompatible);

    // TODO(kinaba): Remove this code for accounts without user prefs.
    // See the comment for |g_known_compatible_users| for the detail.
    if (GetFileSystemCompatibilityPref(account_id) != kFileSystemCompatible)
      g_known_compatible_users.Get().insert(account_id);
  }
  callback.Run();
}

bool IsArcMigrationAllowedInternal(const Profile* profile) {
  policy_util::EcryptfsMigrationAction migration_strategy =
      policy_util::GetDefaultEcryptfsMigrationActionForManagedUser(
          IsActiveDirectoryUserForProfile(profile));
  if (profile->GetPrefs()->IsManagedPreference(
          prefs::kEcryptfsMigrationStrategy)) {
    migration_strategy = static_cast<policy_util::EcryptfsMigrationAction>(
        profile->GetPrefs()->GetInteger(prefs::kEcryptfsMigrationStrategy));
  }
  // |kAskForEcryptfsArcUsers| value is received only if the device is in EDU
  // and admin left the migration policy unset. Note that when enabling ARC on
  // the admin console, it is mandatory for the administrator to also choose a
  // migration policy.
  // In this default case, only a group of devices that had ARC M enabled are
  // allowed to migrate, provided that ARC is enabled by policy.
  // TODO(pmarko): Remove the special kAskForEcryptfsArcUsers handling when we
  // assess that it's not necessary anymore: crbug.com/761348.
  if (migration_strategy ==
      policy_util::EcryptfsMigrationAction::kAskForEcryptfsArcUsers) {
    // Note that ARC enablement is controlled by policy for managed users (as
    // it's marked 'default_for_enterprise_users': False in
    // policy_templates.json).
    DCHECK(profile->GetPrefs()->IsManagedPreference(prefs::kArcEnabled));
    // We can't reuse IsArcPlayStoreEnabledForProfile here because this would
    // lead to a circular dependency: It ends up calling this function for some
    // cases.
    return profile->GetPrefs()->GetBoolean(prefs::kArcEnabled) &&
           base::CommandLine::ForCurrentProcess()->HasSwitch(
               chromeos::switches::kArcTransitionMigrationRequired);
  }

  return migration_strategy !=
         policy_util::EcryptfsMigrationAction::kDisallowMigration;
}

bool IsUnaffiliatedArcAllowed() {
  bool arc_allowed;
  ArcSessionManager* arc_session_manager = ArcSessionManager::Get();
  if (arc_session_manager) {
    switch (arc_session_manager->state()) {
      case ArcSessionManager::State::NOT_INITIALIZED:
      case ArcSessionManager::State::STOPPED:
        // Apply logic below
        break;
      case ArcSessionManager::State::NEGOTIATING_TERMS_OF_SERVICE:
      case ArcSessionManager::State::CHECKING_ANDROID_MANAGEMENT:
      case ArcSessionManager::State::REMOVING_DATA_DIR:
      case ArcSessionManager::State::ACTIVE:
      case ArcSessionManager::State::STOPPING:
        // Never forbid unaffiliated ARC while ARC is running
        return true;
    }
  }
  if (chromeos::CrosSettings::Get()->GetBoolean(
          chromeos::kUnaffiliatedArcAllowed, &arc_allowed)) {
    return arc_allowed;
  }
  // If device policy is not set, allow ARC.
  return true;
}

bool IsArcAllowedForProfileInternal(const Profile* profile,
                                    bool should_report_reason) {
  if (g_disallow_for_testing) {
    VLOG_IF(1, should_report_reason) << "ARC is disallowed for testing.";
    return false;
  }

  // ARC Kiosk can be enabled even if ARC is not yet supported on the device.
  // In that case IsArcKioskMode() should return true as profile is already
  // created.
  if (!IsArcAvailable() && !(IsArcKioskMode() && IsArcKioskAvailable())) {
    VLOG_IF(1, should_report_reason) << "ARC is not available.";
    return false;
  }

  if (!chromeos::ProfileHelper::IsPrimaryProfile(profile)) {
    VLOG_IF(1, should_report_reason)
        << "Non-primary users are not supported in ARC.";
    return false;
  }

  if (profile->IsLegacySupervised()) {
    VLOG_IF(1, should_report_reason)
        << "Supervised users are not supported in ARC.";
    return false;
  }

  if (IsArcBlockedDueToIncompatibleFileSystem(profile) &&
      !IsArcMigrationAllowedByPolicyForProfile(profile)) {
    VLOG_IF(1, should_report_reason)
        << "Incompatible encryption and migration forbidden.";
    return false;
  }

  // Play Store requires an appropriate application install mechanism. Normal
  // users do this through GAIA, but Kiosk and Active Directory users use
  // different application install mechanism. ARC is not allowed otherwise
  // (e.g. in public sessions). cf) crbug.com/605545
  const user_manager::User* user =
      chromeos::ProfileHelper::Get()->GetUserByProfile(profile);
  if (!IsArcAllowedForUser(user)) {
    VLOG_IF(1, should_report_reason) << "ARC is not allowed for the user.";
    return false;
  }

  if (!user->IsAffiliated() && !IsUnaffiliatedArcAllowed()) {
    VLOG_IF(1, should_report_reason)
        << "Device admin disallowed ARC for unaffiliated users.";
    return false;
  }

  // Do not run ARC instance when supervised user is being created.
  // Otherwise noisy notification may be displayed.
  chromeos::UserFlow* user_flow =
      chromeos::ChromeUserManager::Get()->GetUserFlow(user->GetAccountId());
  if (!user_flow || !user_flow->CanStartArc()) {
    VLOG_IF(1, should_report_reason)
        << "ARC is not allowed in the current user flow.";
    return false;
  }

  return true;
}

}  // namespace

bool IsArcAllowedForProfile(const Profile* profile) {
  // Silently ignore default, lock screen and incognito profiles.
  if (!profile || chromeos::ProfileHelper::IsSigninProfile(profile) ||
      profile->IsOffTheRecord() ||
      chromeos::ProfileHelper::IsLockScreenAppProfile(profile)) {
    return false;
  }

  auto it = g_profile_status_check.Get().find(profile);

  const bool first_check = it == g_profile_status_check.Get().end();
  const bool result =
      IsArcAllowedForProfileInternal(profile, first_check /* report_reason */);

  if (first_check) {
    g_profile_status_check.Get()[profile] = result;
    return result;
  }

  // This is next check. We should be persistent and report the same result.
  if (result != it->second) {
    NOTREACHED() << "ARC allowed was changed for the current user session "
                 << "and profile " << profile->GetPath().MaybeAsASCII()
                 << ". This may lead to unexpected behavior. ARC allowed is"
                 << " forced to " << it->second;
  }
  return it->second;
}

void ResetArcAllowedCheckForTesting(const Profile* profile) {
  g_profile_status_check.Get().erase(profile);
}

bool IsArcMigrationAllowedByPolicyForProfile(const Profile* profile) {
  // Always allow migration for unmanaged users.
  if (!profile || !policy_util::IsAccountManaged(profile))
    return true;

  // Use the profile path as unique identifier for profile.
  const base::FilePath path = profile->GetPath();
  auto iter = g_is_arc_migration_allowed.Get().find(path);
  if (iter == g_is_arc_migration_allowed.Get().end()) {
    iter = g_is_arc_migration_allowed.Get()
               .emplace(path, IsArcMigrationAllowedInternal(profile))
               .first;
  }

  return iter->second;
}

bool IsArcBlockedDueToIncompatibleFileSystem(const Profile* profile) {
  const user_manager::User* user =
      chromeos::ProfileHelper::Get()->GetUserByProfile(profile);

  // Return true for public accounts as they only have ext4 and
  // for ARC kiosk as migration to ext4 should always be triggered.
  // Without this check it fails to start after browser crash as
  // compatibility info is stored in RAM.
  if (user && (user->GetType() == user_manager::USER_TYPE_PUBLIC_ACCOUNT ||
               user->GetType() == user_manager::USER_TYPE_ARC_KIOSK_APP)) {
    return false;
  }

  // Test runs on Linux workstation does not have expected /etc/lsb-release
  // field nor profile creation step. Hence it returns a dummy test value.
  if (!base::SysInfo::IsRunningOnChromeOS())
    return g_arc_blocked_due_to_incompatible_filesystem_for_testing;

  // Conducts the actual check, only when running on a real Chrome OS device.
  return !IsArcCompatibleFileSystemUsedForUser(user);
}

void SetArcBlockedDueToIncompatibleFileSystemForTesting(bool block) {
  g_arc_blocked_due_to_incompatible_filesystem_for_testing = block;
}

bool IsArcCompatibleFileSystemUsedForUser(const user_manager::User* user) {
  // Returns false for profiles not associated with users (like sign-in profile)
  if (!user)
    return false;

  // chromeos::UserSessionManager does the actual file system check and stores
  // the result to prefs, so that it survives crash-restart.
  FileSystemCompatibilityState filesystem_compatibility =
      GetFileSystemCompatibilityPref(user->GetAccountId());
  const bool is_filesystem_compatible =
      filesystem_compatibility != kFileSystemIncompatible ||
      g_known_compatible_users.Get().count(user->GetAccountId()) != 0;
  std::string arc_sdk_version;
  const bool is_M = base::SysInfo::GetLsbReleaseValue(kLsbReleaseArcVersionKey,
                                                      &arc_sdk_version) &&
                    arc_sdk_version == kAndroidMSdkVersion;

  // To run ARC we want to make sure either
  // - Underlying file system is compatible with ARC, or
  // - SDK version is M.
  if (!is_filesystem_compatible && !is_M) {
    VLOG(1)
        << "Users with SDK version (" << arc_sdk_version
        << ") are not supported when they postponed to migrate to dircrypto.";
    return false;
  }

  return true;
}

void DisallowArcForTesting() {
  g_disallow_for_testing = true;
  g_profile_status_check.Get().clear();
}

bool IsArcPlayStoreEnabledForProfile(const Profile* profile) {
  return IsArcAllowedForProfile(profile) &&
         profile->GetPrefs()->GetBoolean(prefs::kArcEnabled);
}

bool IsArcPlayStoreEnabledPreferenceManagedForProfile(const Profile* profile) {
  if (!IsArcAllowedForProfile(profile)) {
    LOG(DFATAL) << "ARC is not allowed for profile";
    return false;
  }
  return profile->GetPrefs()->IsManagedPreference(prefs::kArcEnabled);
}

bool SetArcPlayStoreEnabledForProfile(Profile* profile, bool enabled) {
  DCHECK(IsArcAllowedForProfile(profile));
  if (IsArcPlayStoreEnabledPreferenceManagedForProfile(profile)) {
    if (enabled && !IsArcPlayStoreEnabledForProfile(profile)) {
      LOG(WARNING) << "Attempt to enable disabled by policy ARC.";
      return false;
    }
    VLOG(1) << "Google-Play-Store-enabled pref is managed. Request to "
            << (enabled ? "enable" : "disable") << " Play Store is not stored";
    // Need update ARC session manager manually for managed case in order to
    // keep its state up to date, otherwise it may stuck with enabling
    // request.
    // TODO(khmel): Consider finding the better way handling this.
    ArcSessionManager* arc_session_manager = ArcSessionManager::Get();
    // |arc_session_manager| can be nullptr in unit_tests.
    if (!arc_session_manager)
      return false;
    if (enabled)
      arc_session_manager->RequestEnable();
    else
      arc_session_manager->RequestDisable();
    return true;
  }
  profile->GetPrefs()->SetBoolean(prefs::kArcEnabled, enabled);
  return true;
}

bool AreArcAllOptInPreferencesIgnorableForProfile(const Profile* profile) {
  // For Active Directory users, a LaForge account is created, where
  // backup&restore and location services are not supported, hence the policies
  // are unused.
  if (IsActiveDirectoryUserForProfile(profile))
    return true;

  // Otherwise, the preferences are ignorable iff both backup&restore and
  // location services are set off by policy.
  const PrefService* prefs = profile->GetPrefs();
  if (!prefs->IsManagedPreference(prefs::kArcBackupRestoreEnabled) ||
      prefs->GetBoolean(prefs::kArcBackupRestoreEnabled) == true) {
    return false;
  }
  if (!prefs->IsManagedPreference(prefs::kArcLocationServiceEnabled) ||
      prefs->GetBoolean(prefs::kArcLocationServiceEnabled) == true) {
    return false;
  }

  return true;
}

bool IsActiveDirectoryUserForProfile(const Profile* profile) {
  const user_manager::User* user =
      chromeos::ProfileHelper::Get()->GetUserByProfile(profile);
  return user ? user->IsActiveDirectoryUser() : false;
}

bool IsArcOobeOptInActive() {
  // No OOBE is expected in case Play Store is not available.
  if (!IsPlayStoreAvailable())
    return false;

  // Check if Chrome OS OOBE flow is currently showing.
  // TODO(b/65861628): Redesign the OptIn flow since there is no longer reason
  // to have two different OptIn flows.
  if (!chromeos::LoginDisplayHost::default_host())
    return false;

  // Use the legacy logic for first sign-in OOBE OptIn flow. Make sure the user
  // is new.
  if (!user_manager::UserManager::Get()->IsCurrentUserNew())
    return false;

  // Differentiate the case when Assistant Wizard is started later for the new
  // user session. For example, OOBE was shown and user pressed Skip button.
  // Later in the same user session user activates Assistant and we show
  // Assistant Wizard with ARC terms. This case is not considered as OOBE OptIn.
  return !IsArcOptInWizardForAssistantActive();
}

bool IsArcOptInWizardForAssistantActive() {
  // Check if Assistant Wizard is currently showing.
  // TODO(b/65861628): Redesign the OptIn flow since there is no longer reason
  // to have two different OptIn flows.
  chromeos::LoginDisplayHost* host = chromeos::LoginDisplayHost::default_host();
  if (!host || !host->IsVoiceInteractionOobe())
    return false;

  // Make sure the wizard controller is active and have the ARC ToS screen
  // showing for the voice interaction OptIn flow.
  const chromeos::WizardController* wizard_controller =
      host->GetWizardController();
  if (!wizard_controller)
    return false;

  const chromeos::BaseScreen* screen = wizard_controller->current_screen();
  if (!screen)
    return false;
  return screen->screen_id() ==
         chromeos::OobeScreen::SCREEN_ARC_TERMS_OF_SERVICE;
}

bool IsArcTermsOfServiceNegotiationNeeded(const Profile* profile) {
  DCHECK(profile);

  // Skip to show UI asking users to set up ARC OptIn preferences, if all of
  // them are managed by the admin policy. Note that the ToS agreement is anyway
  // not shown in the case of the managed ARC.
  if (IsArcPlayStoreEnabledPreferenceManagedForProfile(profile) &&
      AreArcAllOptInPreferencesIgnorableForProfile(profile)) {
    VLOG(1) << "All opt-in preferences are under managed. "
            << "Skip ARC Terms of Service negotiation.";
    return false;
  }

  // If it is marked that the Terms of service is accepted already,
  // just skip the negotiation with user, and start Android management
  // check directly.
  // This happens, e.g., when a user accepted the Terms of service on Opt-in
  // flow, but logged out before ARC sign in procedure was done. Then, logs
  // in again.
  if (profile->GetPrefs()->GetBoolean(prefs::kArcTermsAccepted)) {
    VLOG(1) << "The user already accepts ARC Terms of Service.";
    return false;
  }

  return true;
}

bool IsArcTermsOfServiceOobeNegotiationNeeded() {
  if (!user_manager::UserManager::Get()->IsUserLoggedIn()) {
    VLOG(1) << "Skip ARC Terms of Service screen because user is not "
            << "logged in.";
    return false;
  }

  const Profile* profile = ProfileManager::GetActiveUserProfile();
  if (!IsArcAllowedForProfile(profile)) {
    VLOG(1) << "Skip ARC Terms of Service screen because ARC is not allowed.";
    return false;
  }

  if (profile->GetPrefs()->IsManagedPreference(prefs::kArcEnabled) &&
      !profile->GetPrefs()->GetBoolean(prefs::kArcEnabled)) {
    VLOG(1) << "Skip ARC Terms of Service screen because ARC is disabled.";
    return false;
  }

  if (!IsPlayStoreAvailable()) {
    VLOG(1) << "Skip ARC Terms of Service screen because Play Store is not "
               "available on the device.";
    return false;
  }

  if (IsActiveDirectoryUserForProfile(profile)) {
    VLOG(1) << "Skip ARC Terms of Service screen because it does not apply to "
               "Active Directory users.";
    return false;
  }

  if (!IsArcTermsOfServiceNegotiationNeeded(profile)) {
    VLOG(1) << "Skip ARC Terms of Service screen because it is already "
               "accepted or fully controlled by policy.";
    return false;
  }

  return true;
}

void UpdateArcFileSystemCompatibilityPrefIfNeeded(
    const AccountId& account_id,
    const base::FilePath& profile_path,
    const base::Closure& callback) {
  DCHECK(!callback.is_null());

  // If ARC is not available, skip the check.
  // This shortcut is just for merginally improving the log-in performance on
  // old devices without ARC. We can always safely remove the following 4 lines
  // without changing any functionality when, say, the code clarity becomes
  // more important in the future.
  if (!IsArcAvailable() && !IsArcKioskAvailable()) {
    callback.Run();
    return;
  }

  // If the compatibility has been already confirmed, skip the check.
  if (GetFileSystemCompatibilityPref(account_id) != kFileSystemIncompatible) {
    callback.Run();
    return;
  }

  // Otherwise, check the underlying filesystem.
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::Bind(&IsArcCompatibleFilesystem, profile_path),
      base::Bind(&StoreCompatibilityCheckResult, account_id, callback));
}

ash::mojom::AssistantAllowedState IsAssistantAllowedForProfile(
    const Profile* profile) {
  if (!chromeos::switches::IsAssistantEnabled() &&
      !chromeos::switches::IsVoiceInteractionFlagsEnabled()) {
    return ash::mojom::AssistantAllowedState::DISALLOWED_BY_FLAG;
  }

  if (!chromeos::ProfileHelper::IsPrimaryProfile(profile))
    return ash::mojom::AssistantAllowedState::DISALLOWED_BY_NONPRIMARY_USER;

  if (profile->IsOffTheRecord())
    return ash::mojom::AssistantAllowedState::DISALLOWED_BY_INCOGNITO;

  if (profile->IsLegacySupervised())
    return ash::mojom::AssistantAllowedState::DISALLOWED_BY_SUPERVISED_USER;

  if (chromeos::switches::IsVoiceInteractionFlagsEnabled()) {
    if (!chromeos::switches::IsVoiceInteractionLocalesSupported())
      return ash::mojom::AssistantAllowedState::DISALLOWED_BY_LOCALE;

    const PrefService* prefs = profile->GetPrefs();
    if (prefs->IsManagedPreference(prefs::kArcEnabled) &&
        !prefs->GetBoolean(prefs::kArcEnabled)) {
      return ash::mojom::AssistantAllowedState::DISALLOWED_BY_ARC_POLICY;
    }

    if (!IsArcAllowedForProfile(profile))
      return ash::mojom::AssistantAllowedState::DISALLOWED_BY_ARC_DISALLOWED;
  }

  return ash::mojom::AssistantAllowedState::ALLOWED;
}

ArcSupervisionTransition GetSupervisionTransition(const Profile* profile) {
  DCHECK(profile);
  DCHECK(profile->GetPrefs());

  const ArcSupervisionTransition supervision_transition =
      static_cast<ArcSupervisionTransition>(
          profile->GetPrefs()->GetInteger(prefs::kArcSupervisionTransition));
  const bool is_child_to_regular_enabled =
      base::FeatureList::IsEnabled(kEnableChildToRegularTransitionFeature);
  const bool is_regular_to_child_enabled =
      base::FeatureList::IsEnabled(kEnableRegularToChildTransitionFeature);

  switch (supervision_transition) {
    case ArcSupervisionTransition::NO_TRANSITION:
      // Do nothing.
      break;
    case ArcSupervisionTransition::CHILD_TO_REGULAR:
      if (!is_child_to_regular_enabled)
        return ArcSupervisionTransition::NO_TRANSITION;
      break;
    case ArcSupervisionTransition::REGULAR_TO_CHILD:
      if (!is_regular_to_child_enabled)
        return ArcSupervisionTransition::NO_TRANSITION;
      break;
  }
  return supervision_transition;
}

}  // namespace arc
