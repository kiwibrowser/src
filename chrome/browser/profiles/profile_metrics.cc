// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_metrics.h"

#include <vector>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/chrome_signin_helper.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/installer/util/google_update_settings.h"
#include "components/profile_metrics/counts.h"
#include "content/public/browser/browser_thread.h"

namespace {

#if defined(OS_WIN) || defined(OS_MACOSX)
const int kMaximumReportedProfileCount = 5;
#endif

const int kMaximumDaysOfDisuse = 4 * 7;  // Should be integral number of weeks.

#if !defined(OS_ANDROID)
size_t number_of_profile_switches_ = 0;
#endif

// Enum for tracking the state of profiles being switched to.
enum ProfileOpenState {
  // Profile being switched to is already opened and has browsers opened.
  PROFILE_OPENED = 0,
  // Profile being switched to is already opened but has no browsers opened.
  PROFILE_OPENED_NO_BROWSER,
  // Profile being switched to is not opened.
  PROFILE_UNOPENED
};

#if !defined(OS_ANDROID)
ProfileOpenState GetProfileOpenState(
    ProfileManager* manager,
    const base::FilePath& path) {
  Profile* profile_switched_to = manager->GetProfileByPath(path);
  if (!profile_switched_to)
    return PROFILE_UNOPENED;

  if (chrome::GetBrowserCount(profile_switched_to) > 0)
    return PROFILE_OPENED;

  return PROFILE_OPENED_NO_BROWSER;
}
#endif

ProfileMetrics::ProfileType GetProfileType(
    const base::FilePath& profile_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  ProfileMetrics::ProfileType metric = ProfileMetrics::SECONDARY;
  ProfileManager* manager = g_browser_process->profile_manager();
  base::FilePath user_data_dir;
  // In unittests, we do not always have a profile_manager so check.
  if (manager) {
    user_data_dir = manager->user_data_dir();
  }
  if (profile_path == user_data_dir.AppendASCII(chrome::kInitialProfile)) {
    metric = ProfileMetrics::ORIGINAL;
  }
  return metric;
}

void LogLockedProfileInformation(ProfileManager* manager) {
  base::Time now = base::Time::Now();
  const int kMinutesInProfileValidDuration =
      base::TimeDelta::FromDays(28).InMinutes();
  std::vector<ProfileAttributesEntry*> entries =
      manager->GetProfileAttributesStorage().GetAllProfilesAttributes();
  for (ProfileAttributesEntry* entry : entries) {
    // Find when locked profiles were locked
    if (entry->IsSigninRequired()) {
      base::TimeDelta time_since_lock = now - entry->GetActiveTime();
      // Specifying 100 buckets for the histogram to get a higher level of
      // granularity in the reported data, given the large number of possible
      // values (kMinutesInProfileValidDuration > 40,000).
      UMA_HISTOGRAM_CUSTOM_COUNTS("Profile.LockedProfilesDuration",
                                  time_since_lock.InMinutes(),
                                  1,
                                  kMinutesInProfileValidDuration,
                                  100);
    }
  }
}

bool HasProfileBeenActiveSince(const ProfileAttributesEntry* entry,
                               const base::Time& active_limit) {
#if !defined(OS_ANDROID)
  // TODO(mlerman): iOS and Android should set an ActiveTime in the
  // ProfileAttributesStorage. (see ProfileManager::OnBrowserSetLastActive)
  if (entry->GetActiveTime() < active_limit)
    return false;
#endif
  return true;
}

}  // namespace

enum ProfileAvatar {
  AVATAR_GENERIC = 0,       // The names for avatar icons
  AVATAR_GENERIC_AQUA,
  AVATAR_GENERIC_BLUE,
  AVATAR_GENERIC_GREEN,
  AVATAR_GENERIC_ORANGE,
  AVATAR_GENERIC_PURPLE,
  AVATAR_GENERIC_RED,
  AVATAR_GENERIC_YELLOW,
  AVATAR_SECRET_AGENT,
  AVATAR_SUPERHERO,
  AVATAR_VOLLEYBALL,        // 10
  AVATAR_BUSINESSMAN,
  AVATAR_NINJA,
  AVATAR_ALIEN,
  AVATAR_AWESOME,
  AVATAR_FLOWER,
  AVATAR_PIZZA,
  AVATAR_SOCCER,
  AVATAR_BURGER,
  AVATAR_CAT,
  AVATAR_CUPCAKE,           // 20
  AVATAR_DOG,
  AVATAR_HORSE,
  AVATAR_MARGARITA,
  AVATAR_NOTE,
  AVATAR_SUN_CLOUD,
  AVATAR_PLACEHOLDER,
  AVATAR_UNKNOWN,           // 27
  AVATAR_GAIA,              // 28
  NUM_PROFILE_AVATAR_METRICS
};

bool ProfileMetrics::CountProfileInformation(ProfileManager* manager,
                                             profile_metrics::Counts* counts) {
  ProfileAttributesStorage& storage = manager->GetProfileAttributesStorage();
  size_t number_of_profiles = storage.GetNumberOfProfiles();
  counts->total = number_of_profiles;

  // Ignore other metrics if we have no profiles.
  if (!number_of_profiles)
    return false;

  // Maximum age for "active" profile is 4 weeks.
  base::Time oldest = base::Time::Now() -
      base::TimeDelta::FromDays(kMaximumDaysOfDisuse);

  std::vector<ProfileAttributesEntry*> entries =
      storage.GetAllProfilesAttributes();
  for (ProfileAttributesEntry* entry : entries) {
    if (!HasProfileBeenActiveSince(entry, oldest)) {
      counts->unused++;
    } else {
      counts->active++;
      if (entry->IsSupervised())
        counts->supervised++;
      if (entry->IsAuthenticated()) {
        counts->signedin++;
        if (entry->IsUsingGAIAPicture())
          counts->gaia_icon++;
        if (entry->IsAuthError())
          counts->auth_errors++;
      }
    }
  }
  return true;
}

void ProfileMetrics::UpdateReportedProfilesStatistics(ProfileManager* manager) {
#if defined(OS_WIN) || defined(OS_MACOSX)
  profile_metrics::Counts counts;
  if (CountProfileInformation(manager, &counts)) {
    size_t limited_total = counts.total;
    size_t limited_signedin = counts.signedin;
    if (limited_total > kMaximumReportedProfileCount) {
      limited_total = kMaximumReportedProfileCount + 1;
      limited_signedin =
          (int)((float)(counts.signedin * limited_total)
          / counts.total + 0.5);
    }
    UpdateReportedOSProfileStatistics(limited_total, limited_signedin);
  }
#endif
}

#if !defined(OS_ANDROID)
void ProfileMetrics::LogNumberOfProfileSwitches() {
  UMA_HISTOGRAM_COUNTS_100("Profile.NumberOfSwitches",
                           number_of_profile_switches_);
}
#endif

// The OS_MACOSX implementation of this function is in profile_metrics_mac.mm.
#if defined(OS_WIN)
void ProfileMetrics::UpdateReportedOSProfileStatistics(
    size_t active, size_t signedin) {
  GoogleUpdateSettings::UpdateProfileCounts(active, signedin);
}
#endif

void ProfileMetrics::LogNumberOfProfiles(ProfileManager* manager) {
  profile_metrics::Counts counts;
  bool success = CountProfileInformation(manager, &counts);

  profile_metrics::LogProfileMetricsCounts(counts);

  // Ignore other metrics if we have no profiles.
  if (success) {
    LogLockedProfileInformation(manager);

#if defined(OS_WIN) || defined(OS_MACOSX)
    UpdateReportedOSProfileStatistics(counts.total, counts.signedin);
#endif
  }
}

void ProfileMetrics::LogProfileAddNewUser(ProfileAdd metric) {
  DCHECK(metric < NUM_PROFILE_ADD_METRICS);
  UMA_HISTOGRAM_ENUMERATION("Profile.AddNewUser", metric,
                            NUM_PROFILE_ADD_METRICS);
  UMA_HISTOGRAM_ENUMERATION("Profile.NetUserCount", ADD_NEW_USER,
                            NUM_PROFILE_NET_METRICS);
}

void ProfileMetrics::LogProfileAvatarSelection(size_t icon_index) {
  DCHECK(icon_index < NUM_PROFILE_AVATAR_METRICS);
  ProfileAvatar icon_name = AVATAR_UNKNOWN;
  switch (icon_index) {
    case 0:
      icon_name = AVATAR_GENERIC;
      break;
    case 1:
      icon_name = AVATAR_GENERIC_AQUA;
      break;
    case 2:
      icon_name = AVATAR_GENERIC_BLUE;
      break;
    case 3:
      icon_name = AVATAR_GENERIC_GREEN;
      break;
    case 4:
      icon_name = AVATAR_GENERIC_ORANGE;
      break;
    case 5:
      icon_name = AVATAR_GENERIC_PURPLE;
      break;
    case 6:
      icon_name = AVATAR_GENERIC_RED;
      break;
    case 7:
      icon_name = AVATAR_GENERIC_YELLOW;
      break;
    case 8:
      icon_name = AVATAR_SECRET_AGENT;
      break;
    case 9:
      icon_name = AVATAR_SUPERHERO;
      break;
    case 10:
      icon_name = AVATAR_VOLLEYBALL;
      break;
    case 11:
      icon_name = AVATAR_BUSINESSMAN;
      break;
    case 12:
      icon_name = AVATAR_NINJA;
      break;
    case 13:
      icon_name = AVATAR_ALIEN;
      break;
    case 14:
      icon_name = AVATAR_AWESOME;
      break;
    case 15:
      icon_name = AVATAR_FLOWER;
      break;
    case 16:
      icon_name = AVATAR_PIZZA;
      break;
    case 17:
      icon_name = AVATAR_SOCCER;
      break;
    case 18:
      icon_name = AVATAR_BURGER;
      break;
    case 19:
      icon_name = AVATAR_CAT;
      break;
    case 20:
      icon_name = AVATAR_CUPCAKE;
      break;
    case 21:
      icon_name = AVATAR_DOG;
      break;
    case 22:
      icon_name = AVATAR_HORSE;
      break;
    case 23:
      icon_name = AVATAR_MARGARITA;
      break;
    case 24:
      icon_name = AVATAR_NOTE;
      break;
    case 25:
      icon_name = AVATAR_SUN_CLOUD;
      break;
    case 26:
      icon_name = AVATAR_PLACEHOLDER;
      break;
    case 28:
      icon_name = AVATAR_GAIA;
      break;
    default:  // We should never actually get here.
      NOTREACHED();
      break;
  }
  UMA_HISTOGRAM_ENUMERATION("Profile.Avatar", icon_name,
                            NUM_PROFILE_AVATAR_METRICS);
}

void ProfileMetrics::LogProfileDeleteUser(ProfileDelete metric) {
  DCHECK(metric < NUM_DELETE_PROFILE_METRICS);
  UMA_HISTOGRAM_ENUMERATION("Profile.DeleteProfileAction", metric,
                            NUM_DELETE_PROFILE_METRICS);
  if (metric != DELETE_PROFILE_USER_MANAGER_SHOW_WARNING &&
      metric != DELETE_PROFILE_SETTINGS_SHOW_WARNING &&
      metric != DELETE_PROFILE_ABORTED) {
    // If a user was actually deleted, update the net user count.
    UMA_HISTOGRAM_ENUMERATION("Profile.NetUserCount", PROFILE_DELETED,
                              NUM_PROFILE_NET_METRICS);
  }
}

void ProfileMetrics::LogProfileOpenMethod(ProfileOpen metric) {
  DCHECK(metric < NUM_PROFILE_OPEN_METRICS);
  UMA_HISTOGRAM_ENUMERATION("Profile.OpenMethod", metric,
                            NUM_PROFILE_OPEN_METRICS);
}

#if !defined(OS_ANDROID)
void ProfileMetrics::LogProfileSwitch(
    ProfileOpen metric,
    ProfileManager* manager,
    const base::FilePath& profile_path) {
  DCHECK(metric < NUM_PROFILE_OPEN_METRICS);
  ProfileOpenState open_state = GetProfileOpenState(manager, profile_path);
  switch (open_state) {
    case PROFILE_OPENED:
      UMA_HISTOGRAM_ENUMERATION(
        "Profile.OpenMethod.ToOpenedProfile",
        metric,
        NUM_PROFILE_OPEN_METRICS);
      break;
    case PROFILE_OPENED_NO_BROWSER:
      UMA_HISTOGRAM_ENUMERATION(
        "Profile.OpenMethod.ToOpenedProfileWithoutBrowser",
        metric,
        NUM_PROFILE_OPEN_METRICS);
      break;
    case PROFILE_UNOPENED:
      UMA_HISTOGRAM_ENUMERATION(
        "Profile.OpenMethod.ToUnopenedProfile",
        metric,
        NUM_PROFILE_OPEN_METRICS);
      break;
    default:
      // There are no other possible values.
      NOTREACHED();
      break;
  }

  ++number_of_profile_switches_;
  // The LogOpenMethod histogram aggregates data from profile switches as well
  // as opening of profile related UI elements.
  LogProfileOpenMethod(metric);
}
#endif

void ProfileMetrics::LogProfileSwitchGaia(ProfileGaia metric) {
  if (metric == GAIA_OPT_IN)
    LogProfileAvatarSelection(AVATAR_GAIA);
  UMA_HISTOGRAM_ENUMERATION("Profile.SwitchGaiaPhotoSettings",
                            metric,
                            NUM_PROFILE_GAIA_METRICS);
}

void ProfileMetrics::LogProfileSyncInfo(ProfileSync metric) {
  DCHECK(metric < NUM_PROFILE_SYNC_METRICS);
  UMA_HISTOGRAM_ENUMERATION("Profile.SyncCustomize", metric,
                            NUM_PROFILE_SYNC_METRICS);
}

void ProfileMetrics::LogProfileAuthResult(ProfileAuth metric) {
  UMA_HISTOGRAM_ENUMERATION("Profile.AuthResult", metric,
                            NUM_PROFILE_AUTH_METRICS);
}

void ProfileMetrics::LogProfileDesktopMenu(
    ProfileDesktopMenu metric,
    signin::GAIAServiceType gaia_service) {
  // The first parameter to the histogram needs to be literal, because of the
  // optimized implementation of |UMA_HISTOGRAM_ENUMERATION|. Do not attempt
  // to refactor.
  switch (gaia_service) {
    case signin::GAIA_SERVICE_TYPE_NONE:
      UMA_HISTOGRAM_ENUMERATION("Profile.DesktopMenu.NonGAIA", metric,
                                NUM_PROFILE_DESKTOP_MENU_METRICS);
      break;
    case signin::GAIA_SERVICE_TYPE_SIGNOUT:
      UMA_HISTOGRAM_ENUMERATION("Profile.DesktopMenu.GAIASignout", metric,
                                NUM_PROFILE_DESKTOP_MENU_METRICS);
      break;
    case signin::GAIA_SERVICE_TYPE_INCOGNITO:
      UMA_HISTOGRAM_ENUMERATION("Profile.DesktopMenu.GAIAIncognito",
                                metric, NUM_PROFILE_DESKTOP_MENU_METRICS);
      break;
    case signin::GAIA_SERVICE_TYPE_ADDSESSION:
      UMA_HISTOGRAM_ENUMERATION("Profile.DesktopMenu.GAIAAddSession", metric,
                                NUM_PROFILE_DESKTOP_MENU_METRICS);
      break;
    case signin::GAIA_SERVICE_TYPE_REAUTH:
      UMA_HISTOGRAM_ENUMERATION("Profile.DesktopMenu.GAIAReAuth", metric,
                                NUM_PROFILE_DESKTOP_MENU_METRICS);
      break;
    case signin::GAIA_SERVICE_TYPE_SIGNUP:
      UMA_HISTOGRAM_ENUMERATION("Profile.DesktopMenu.GAIASignup", metric,
                                NUM_PROFILE_DESKTOP_MENU_METRICS);
      break;
    case signin::GAIA_SERVICE_TYPE_DEFAULT:
      UMA_HISTOGRAM_ENUMERATION("Profile.DesktopMenu.GAIADefault", metric,
                                NUM_PROFILE_DESKTOP_MENU_METRICS);
      break;
  }
}

void ProfileMetrics::LogProfileDelete(bool profile_was_signed_in) {
  UMA_HISTOGRAM_BOOLEAN("Profile.Delete", profile_was_signed_in);
}

void ProfileMetrics::LogTimeToOpenUserManager(
    const base::TimeDelta& time_to_open) {
  UMA_HISTOGRAM_TIMES("Profile.TimeToOpenUserManager", time_to_open);
}

#if defined(OS_ANDROID)
void ProfileMetrics::LogProfileAndroidAccountManagementMenu(
    ProfileAndroidAccountManagementMenu metric,
    signin::GAIAServiceType gaia_service) {
  // The first parameter to the histogram needs to be literal, because of the
  // optimized implementation of |UMA_HISTOGRAM_ENUMERATION|. Do not attempt
  // to refactor.
  switch (gaia_service) {
    case signin::GAIA_SERVICE_TYPE_NONE:
      UMA_HISTOGRAM_ENUMERATION(
          "Profile.AndroidAccountManagementMenu.NonGAIA",
          metric,
          NUM_PROFILE_ANDROID_ACCOUNT_MANAGEMENT_MENU_METRICS);
      break;
    case signin::GAIA_SERVICE_TYPE_SIGNOUT:
      UMA_HISTOGRAM_ENUMERATION(
          "Profile.AndroidAccountManagementMenu.GAIASignout",
          metric,
          NUM_PROFILE_ANDROID_ACCOUNT_MANAGEMENT_MENU_METRICS);
      break;
    case signin::GAIA_SERVICE_TYPE_INCOGNITO:
      UMA_HISTOGRAM_ENUMERATION(
          "Profile.AndroidAccountManagementMenu.GAIASignoutIncognito",
          metric,
          NUM_PROFILE_ANDROID_ACCOUNT_MANAGEMENT_MENU_METRICS);
      break;
    case signin::GAIA_SERVICE_TYPE_ADDSESSION:
      UMA_HISTOGRAM_ENUMERATION(
          "Profile.AndroidAccountManagementMenu.GAIAAddSession",
          metric,
          NUM_PROFILE_ANDROID_ACCOUNT_MANAGEMENT_MENU_METRICS);
      break;
    case signin::GAIA_SERVICE_TYPE_REAUTH:
      UMA_HISTOGRAM_ENUMERATION(
          "Profile.AndroidAccountManagementMenu.GAIAReAuth",
          metric,
          NUM_PROFILE_ANDROID_ACCOUNT_MANAGEMENT_MENU_METRICS);
      break;
    case signin::GAIA_SERVICE_TYPE_SIGNUP:
      UMA_HISTOGRAM_ENUMERATION(
          "Profile.AndroidAccountManagementMenu.GAIASignup",
          metric,
          NUM_PROFILE_ANDROID_ACCOUNT_MANAGEMENT_MENU_METRICS);
      break;
    case signin::GAIA_SERVICE_TYPE_DEFAULT:
      UMA_HISTOGRAM_ENUMERATION(
          "Profile.AndroidAccountManagementMenu.GAIADefault",
          metric,
          NUM_PROFILE_ANDROID_ACCOUNT_MANAGEMENT_MENU_METRICS);
      break;
  }
}
#endif  // defined(OS_ANDROID)

void ProfileMetrics::LogProfileLaunch(Profile* profile) {
  base::FilePath profile_path = profile->GetPath();
  UMA_HISTOGRAM_ENUMERATION("Profile.LaunchBrowser",
                            GetProfileType(profile_path),
                            NUM_PROFILE_TYPE_METRICS);

  if (profile->IsSupervised()) {
    base::RecordAction(
        base::UserMetricsAction("ManagedMode_NewManagedUserWindow"));
  }
}

void ProfileMetrics::LogProfileSyncSignIn(const base::FilePath& profile_path) {
  UMA_HISTOGRAM_ENUMERATION("Profile.SyncSignIn",
                            GetProfileType(profile_path),
                            NUM_PROFILE_TYPE_METRICS);
}

void ProfileMetrics::LogProfileUpdate(const base::FilePath& profile_path) {
  UMA_HISTOGRAM_ENUMERATION("Profile.Update",
                            GetProfileType(profile_path),
                            NUM_PROFILE_TYPE_METRICS);
}
