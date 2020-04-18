// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/system/system_clock.h"

#include <memory>

#include "base/logging.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/ownership/owner_settings_service_chromeos.h"
#include "chrome/browser/chromeos/ownership/owner_settings_service_chromeos_factory.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/system/system_clock_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "chromeos/login/login_state.h"
#include "chromeos/settings/cros_settings_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/notification_service.h"

namespace chromeos {
namespace system {

namespace {

void SetShouldUse24HourClock(bool use_24_hour_clock) {
  user_manager::User* const user =
      user_manager::UserManager::Get()->GetActiveUser();
  if (!user)
    return;  // May occur if not running on a device.
  Profile* const profile = ProfileHelper::Get()->GetProfileByUser(user);
  if (!profile)
    return;  // May occur in tests or if not running on a device.
  OwnerSettingsServiceChromeOS* const service =
      OwnerSettingsServiceChromeOSFactory::GetForBrowserContext(profile);
  CHECK(service);
  service->SetBoolean(kSystemUse24HourClock, use_24_hour_clock);
}

}  // anonymous namespace

SystemClock::SystemClock()
    : user_pod_was_focused_(false),
      last_focused_pod_hour_clock_type_(base::k12HourClock),
      user_profile_(NULL),
      device_settings_observer_(CrosSettings::Get()->AddSettingsObserver(
          kSystemUse24HourClock,
          base::Bind(&SystemClock::OnSystemPrefChanged,
                     base::Unretained(this)))) {
  if (LoginState::IsInitialized())
    LoginState::Get()->AddObserver(this);
  // Register notifications on construction so that events such as
  // PROFILE_CREATED do not get missed if they happen before Initialize().
  registrar_.reset(new content::NotificationRegistrar);
  if (!LoginState::IsInitialized() ||
      LoginState::Get()->GetLoggedInUserType() ==
          LoginState::LOGGED_IN_USER_NONE) {
    registrar_->Add(this, chrome::NOTIFICATION_SESSION_STARTED,
                    content::NotificationService::AllSources());
  }
  registrar_->Add(this, chrome::NOTIFICATION_PROFILE_CREATED,
                  content::NotificationService::AllSources());
  registrar_->Add(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                  content::NotificationService::AllSources());
  registrar_->Add(this, chrome::NOTIFICATION_LOGIN_USER_PROFILE_PREPARED,
                  content::NotificationService::AllSources());
  user_manager::UserManager::Get()->AddSessionStateObserver(this);
}

SystemClock::~SystemClock() {
  registrar_.reset();
  device_settings_observer_.reset();
  if (LoginState::IsInitialized())
    LoginState::Get()->RemoveObserver(this);
  if (user_manager::UserManager::IsInitialized())
    user_manager::UserManager::Get()->RemoveSessionStateObserver(this);
}

// LoginState::Observer overrides.
void SystemClock::LoggedInStateChanged() {
  // It apparently sometimes takes a while after login before the current user
  // is recognized as the owner. Make sure that the system-wide clock setting
  // is updated when the recognition eventually happens (crbug.com/278601).
  if (user_manager::UserManager::Get()->IsCurrentUserOwner())
    SetShouldUse24HourClock(ShouldUse24HourClock());
}

// content::NotificationObserver implementation.
void SystemClock::Observe(int type,
                          const content::NotificationSource& source,
                          const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_LOGIN_USER_PROFILE_PREPARED: {
      UpdateClockType();
      break;
    }
    case chrome::NOTIFICATION_PROFILE_CREATED: {
      OnActiveProfileChanged(content::Source<Profile>(source).ptr());
      registrar_->Remove(this, chrome::NOTIFICATION_PROFILE_CREATED,
                         content::NotificationService::AllSources());
      break;
    }
    case chrome::NOTIFICATION_PROFILE_DESTROYED: {
      if (OnProfileDestroyed(content::Source<Profile>(source).ptr())) {
        registrar_->Remove(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                           content::NotificationService::AllSources());
      }
      break;
    }
    case chrome::NOTIFICATION_SESSION_STARTED: {
      OnActiveProfileChanged(ProfileManager::GetActiveUserProfile());
      break;
    }
    default:
      NOTREACHED();
  }
}

void SystemClock::ActiveUserChanged(const user_manager::User* active_user) {
  if (active_user && active_user->is_profile_created())
    UpdateClockType();
}

void SystemClock::AddObserver(SystemClockObserver* observer) {
  observer_list_.AddObserver(observer);
}

void SystemClock::RemoveObserver(SystemClockObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

void SystemClock::OnActiveProfileChanged(Profile* profile) {
  user_profile_ = profile;
  PrefService* prefs = profile->GetPrefs();
  user_pref_registrar_.reset(new PrefChangeRegistrar);
  user_pref_registrar_->Init(prefs);
  user_pref_registrar_->Add(
      prefs::kUse24HourClock,
      base::Bind(&SystemClock::UpdateClockType, base::Unretained(this)));
  UpdateClockType();
}

bool SystemClock::OnProfileDestroyed(Profile* profile) {
  if (profile != user_profile_)
    return false;
  user_pref_registrar_.reset();
  user_profile_ = NULL;
  return true;
}

void SystemClock::SetLastFocusedPodHourClockType(
    base::HourClockType hour_clock_type) {
  user_pod_was_focused_ = true;
  last_focused_pod_hour_clock_type_ = hour_clock_type;
  UpdateClockType();
}

bool SystemClock::ShouldUse24HourClock() const {
  // On login screen and in guest mode owner default is used for
  // kUse24HourClock preference.
  const chromeos::LoginState::LoggedInUserType status =
      LoginState::IsInitialized() ? LoginState::Get()->GetLoggedInUserType()
                                  : LoginState::LOGGED_IN_USER_NONE;

  if (status == LoginState::LOGGED_IN_USER_NONE && user_pod_was_focused_)
    return last_focused_pod_hour_clock_type_ == base::k24HourClock;

  const CrosSettings* const cros_settings = CrosSettings::Get();
  bool system_use_24_hour_clock = true;
  const bool system_value_found = cros_settings->GetBoolean(
      kSystemUse24HourClock, &system_use_24_hour_clock);

  if ((status == LoginState::LOGGED_IN_USER_NONE) || !user_pref_registrar_) {
    return (system_value_found
                ? system_use_24_hour_clock
                : (base::GetHourClockType() == base::k24HourClock));
  }

  const PrefService::Preference* user_pref =
      user_pref_registrar_->prefs()->FindPreference(prefs::kUse24HourClock);
  if (status == LoginState::LOGGED_IN_USER_GUEST &&
      user_pref->IsDefaultValue()) {
    return (system_value_found
                ? system_use_24_hour_clock
                : (base::GetHourClockType() == base::k24HourClock));
  }

  user_manager::User* active_user =
      user_manager::UserManager::Get()->GetActiveUser();
  if (active_user) {
    Profile* user_profile = ProfileHelper::Get()->GetProfileByUser(active_user);
    if (user_profile) {
      user_pref =
          user_profile->GetPrefs()->FindPreference(prefs::kUse24HourClock);
    }
  }

  bool use_24_hour_clock = true;
  user_pref->GetValue()->GetAsBoolean(&use_24_hour_clock);
  return use_24_hour_clock;
}

void SystemClock::OnSystemPrefChanged() {
  UpdateClockType();
}

void SystemClock::UpdateClockType() {
  // This also works for enterprise-managed devices because they never have
  // a local owner.
  if (user_manager::UserManager::Get()->IsCurrentUserOwner())
    SetShouldUse24HourClock(ShouldUse24HourClock());
  for (auto& observer : observer_list_)
    observer.OnSystemClockChanged(this);
}

}  // namespace system
}  // namespace chromeos
