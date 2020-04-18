// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/child_accounts/screen_time_controller.h"

#include "ash/public/cpp/vector_icons/vector_icons.h"
#include "base/optional.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/login_screen_client.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/session_manager/core/session_manager.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/time_format.h"
#include "ui/message_center/public/cpp/notification.h"

namespace chromeos {

namespace {

constexpr base::TimeDelta kWarningNotificationTimeout =
    base::TimeDelta::FromMinutes(5);
constexpr base::TimeDelta kExitNotificationTimeout =
    base::TimeDelta::FromMinutes(1);
constexpr base::TimeDelta kScreenTimeUsageUpdateFrequency =
    base::TimeDelta::FromMinutes(1);

// The notification id. All the time limit notifications share the same id so
// that a subsequent notification can replace the previous one.
constexpr char kTimeLimitNotificationId[] = "time-limit-notification";

// The notifier id representing the app.
constexpr char kTimeLimitNotifierId[] = "family-link";

// Dictionary keys for prefs::kScreenTimeLastState. Time relavant states are
// not saved because processor should not count on them as they could become
// invalid easiliy.
constexpr char kScreenStateLocked[] = "locked";
constexpr char kScreenStateCurrentPolicyType[] = "active_policy";
constexpr char kScreenStateTimeUsageLimitEnabled[] = "time_usage_limit_enabled";
constexpr char kScreenStateNextPolicyType[] = "next_active_policy";

}  // namespace

// static
void ScreenTimeController::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(prefs::kCurrentScreenStartTime, base::Time());
  registry->RegisterTimePref(prefs::kFirstScreenStartTime, base::Time());
  registry->RegisterIntegerPref(prefs::kScreenTimeMinutesUsed, 0);
  registry->RegisterDictionaryPref(prefs::kUsageTimeLimit);
  registry->RegisterDictionaryPref(prefs::kScreenTimeLastState);
}

ScreenTimeController::ScreenTimeController(content::BrowserContext* context)
    : context_(context),
      pref_service_(Profile::FromBrowserContext(context)->GetPrefs()) {
  session_manager::SessionManager::Get()->AddObserver(this);
  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.Add(
      prefs::kUsageTimeLimit,
      base::BindRepeating(&ScreenTimeController::OnPolicyChanged,
                          base::Unretained(this)));
}

ScreenTimeController::~ScreenTimeController() {
  session_manager::SessionManager::Get()->RemoveObserver(this);
  SaveScreenTimeProgressBeforeExit();
}

base::TimeDelta ScreenTimeController::GetScreenTimeDuration() const {
  base::TimeDelta previous_duration = base::TimeDelta::FromMinutes(
      pref_service_->GetInteger(prefs::kScreenTimeMinutesUsed));
  if (current_screen_start_time_.is_null())
    return previous_duration;

  base::TimeDelta current_screen_duration =
      base::Time::Now() - current_screen_start_time_;
  return current_screen_duration + previous_duration;
}

void ScreenTimeController::CheckTimeLimit() {
  // Stop any currently running timer.
  ResetTimers();

  base::Time now = base::Time::Now();
  const base::DictionaryValue* time_limit =
      pref_service_->GetDictionary(prefs::kUsageTimeLimit);
  usage_time_limit::State state = usage_time_limit::GetState(
      time_limit->CreateDeepCopy(), GetScreenTimeDuration(),
      first_screen_start_time_, now, GetLastStateFromPref());
  SaveCurrentStateToPref(state);

  if (state.is_locked) {
    base::Time reset_time = usage_time_limit::GetExpectedResetTime(
        time_limit->CreateDeepCopy(), now);
    LockScreen(true /*force_lock_by_policy*/, reset_time);
  } else {
    usage_time_limit::ActivePolicies active_policy = state.active_policy;
    if (active_policy == usage_time_limit::ActivePolicies::kNoActivePolicy)
      RefreshScreenLimit();
    LockScreen(false /*force_lock_by_policy*/, base::Time() /*come_back_time*/);

    if (!state.next_state_change_time.is_null() &&
        (active_policy == usage_time_limit::ActivePolicies::kFixedLimit ||
         active_policy == usage_time_limit::ActivePolicies::kUsageLimit)) {
      // Schedule notification based on the remaining screen time.
      const base::TimeDelta remaining_usage = state.remaining_usage;
      const TimeLimitNotificationType notification_type =
          active_policy == usage_time_limit::ActivePolicies::kFixedLimit
              ? kBedTime
              : kScreenTime;

      if (remaining_usage >= kWarningNotificationTimeout) {
        warning_notification_timer_.Start(
            FROM_HERE, remaining_usage - kWarningNotificationTimeout,
            base::BindRepeating(&ScreenTimeController::ShowNotification,
                                base::Unretained(this), notification_type,
                                kWarningNotificationTimeout));
      }

      if (remaining_usage >= kExitNotificationTimeout) {
        exit_notification_timer_.Start(
            FROM_HERE, remaining_usage - kExitNotificationTimeout,
            base::BindRepeating(&ScreenTimeController::ShowNotification,
                                base::Unretained(this), notification_type,
                                kExitNotificationTimeout));
      }
    }
  }

  if (!state.next_state_change_time.is_null()) {
    next_state_timer_.Start(
        FROM_HERE, state.next_state_change_time - base::Time::Now(),
        base::BindRepeating(&ScreenTimeController::CheckTimeLimit,
                            base::Unretained(this)));
  }
}

void ScreenTimeController::LockScreen(bool force_lock_by_policy,
                                      base::Time come_back_time) {
  bool is_locked = session_manager::SessionManager::Get()->IsScreenLocked();
  // No-op if the screen is currently not locked and policy does not force the
  // lock.
  if (!is_locked && !force_lock_by_policy)
    return;

  // Request to show lock screen.
  if (!is_locked && force_lock_by_policy) {
    chromeos::DBusThreadManager::Get()
        ->GetSessionManagerClient()
        ->RequestLockScreen();
  }

  AccountId account_id =
      chromeos::ProfileHelper::Get()
          ->GetUserByProfile(Profile::FromBrowserContext(context_))
          ->GetAccountId();
  LoginScreenClient::Get()->login_screen()->SetAuthEnabledForUser(
      account_id, !force_lock_by_policy,
      force_lock_by_policy ? come_back_time : base::Optional<base::Time>());
}

void ScreenTimeController::ShowNotification(
    ScreenTimeController::TimeLimitNotificationType type,
    const base::TimeDelta& time_remaining) {
  const base::string16 title = l10n_util::GetStringUTF16(
      type == kScreenTime ? IDS_SCREEN_TIME_NOTIFICATION_TITLE
                          : IDS_BED_TIME_NOTIFICATION_TITLE);
  std::unique_ptr<message_center::Notification> notification =
      message_center::Notification::CreateSystemNotification(
          message_center::NOTIFICATION_TYPE_SIMPLE, kTimeLimitNotificationId,
          title,
          ui::TimeFormat::Simple(ui::TimeFormat::FORMAT_DURATION,
                                 ui::TimeFormat::LENGTH_LONG, time_remaining),
          gfx::Image(),
          l10n_util::GetStringUTF16(IDS_TIME_LIMIT_NOTIFICATION_DISPLAY_SOURCE),
          GURL(),
          message_center::NotifierId(
              message_center::NotifierId::SYSTEM_COMPONENT,
              kTimeLimitNotifierId),
          message_center::RichNotificationData(),
          new message_center::NotificationDelegate(),
          ash::kNotificationSupervisedUserIcon,
          message_center::SystemNotificationWarningLevel::NORMAL);
  NotificationDisplayService::GetForProfile(
      Profile::FromBrowserContext(context_))
      ->Display(NotificationHandler::Type::TRANSIENT, *notification);
}

void ScreenTimeController::RefreshScreenLimit() {
  base::Time now = base::Time::Now();
  pref_service_->SetTime(prefs::kFirstScreenStartTime, now);
  pref_service_->SetTime(prefs::kCurrentScreenStartTime, now);
  pref_service_->SetInteger(prefs::kScreenTimeMinutesUsed, 0);
  pref_service_->CommitPendingWrite();

  first_screen_start_time_ = now;
  current_screen_start_time_ = now;
}

void ScreenTimeController::OnPolicyChanged() {
  CheckTimeLimit();
}

void ScreenTimeController::ResetTimers() {
  next_state_timer_.Stop();
  warning_notification_timer_.Stop();
  exit_notification_timer_.Stop();
  save_screen_time_timer_.Stop();
}

void ScreenTimeController::SaveScreenTimeProgressBeforeExit() {
  pref_service_->SetInteger(prefs::kScreenTimeMinutesUsed,
                            GetScreenTimeDuration().InMinutes());
  pref_service_->ClearPref(prefs::kCurrentScreenStartTime);
  pref_service_->CommitPendingWrite();
  current_screen_start_time_ = base::Time();
  ResetTimers();
}

void ScreenTimeController::SaveScreenTimeProgressPeriodically() {
  pref_service_->SetInteger(prefs::kScreenTimeMinutesUsed,
                            GetScreenTimeDuration().InMinutes());
  current_screen_start_time_ = base::Time::Now();
  pref_service_->SetTime(prefs::kCurrentScreenStartTime,
                         current_screen_start_time_);
  pref_service_->CommitPendingWrite();
}

void ScreenTimeController::SaveCurrentStateToPref(
    const usage_time_limit::State& state) {
  auto state_dict =
      std::make_unique<base::Value>(base::Value::Type::DICTIONARY);

  state_dict->SetKey(kScreenStateLocked, base::Value(state.is_locked));
  state_dict->SetKey(kScreenStateCurrentPolicyType,
                     base::Value(static_cast<int>(state.active_policy)));
  state_dict->SetKey(kScreenStateTimeUsageLimitEnabled,
                     base::Value(state.is_time_usage_limit_enabled));
  state_dict->SetKey(
      kScreenStateNextPolicyType,
      base::Value(static_cast<int>(state.next_state_active_policy)));

  pref_service_->Set(prefs::kScreenTimeLastState, *state_dict);
  pref_service_->CommitPendingWrite();
}

base::Optional<usage_time_limit::State>
ScreenTimeController::GetLastStateFromPref() {
  const base::DictionaryValue* last_state =
      pref_service_->GetDictionary(prefs::kScreenTimeLastState);
  usage_time_limit::State result;
  if (last_state->empty())
    return base::nullopt;

  // Verify is_locked from the pref is a boolean value.
  const base::Value* is_locked = last_state->FindKey(kScreenStateLocked);
  if (!is_locked || !is_locked->is_bool())
    return base::nullopt;
  result.is_locked = is_locked->GetBool();

  // Verify active policy type is a value of usage_time_limit::ActivePolicies.
  const base::Value* active_policy =
      last_state->FindKey(kScreenStateCurrentPolicyType);
  // TODO(crbug.com/823536): Add kCount in usage_time_limit::ActivePolicies
  // instead of checking kUsageLimit here.
  if (!active_policy || !active_policy->is_int() ||
      active_policy->GetInt() < 0 ||
      active_policy->GetInt() >
          static_cast<int>(usage_time_limit::ActivePolicies::kUsageLimit)) {
    return base::nullopt;
  }
  result.active_policy =
      static_cast<usage_time_limit::ActivePolicies>(active_policy->GetInt());

  // Verify time_usage_limit_enabled from the pref is a boolean value.
  const base::Value* time_usage_limit_enabled =
      last_state->FindKey(kScreenStateTimeUsageLimitEnabled);
  if (!time_usage_limit_enabled || !time_usage_limit_enabled->is_bool())
    return base::nullopt;
  result.is_time_usage_limit_enabled = time_usage_limit_enabled->GetBool();

  // Verify next policy type is a value of usage_time_limit::ActivePolicies.
  const base::Value* next_active_policy =
      last_state->FindKey(kScreenStateNextPolicyType);
  if (!next_active_policy || !next_active_policy->is_int() ||
      next_active_policy->GetInt() < 0 ||
      next_active_policy->GetInt() >
          static_cast<int>(usage_time_limit::ActivePolicies::kUsageLimit)) {
    return base::nullopt;
  }
  result.next_state_active_policy =
      static_cast<usage_time_limit::ActivePolicies>(
          next_active_policy->GetInt());
  return result;
}

void ScreenTimeController::OnSessionStateChanged() {
  session_manager::SessionState session_state =
      session_manager::SessionManager::Get()->session_state();
  if (session_state == session_manager::SessionState::LOCKED) {
    SaveScreenTimeProgressBeforeExit();
  } else if (session_state == session_manager::SessionState::ACTIVE) {
    base::Time now = base::Time::Now();
    const base::Time first_screen_start_time =
        pref_service_->GetTime(prefs::kFirstScreenStartTime);
    if (first_screen_start_time.is_null()) {
      pref_service_->SetTime(prefs::kFirstScreenStartTime, now);
      first_screen_start_time_ = now;
    } else {
      first_screen_start_time_ = first_screen_start_time;
    }

    const base::Time current_screen_start_time =
        pref_service_->GetTime(prefs::kCurrentScreenStartTime);
    if (!current_screen_start_time.is_null() &&
        current_screen_start_time < now &&
        (now - current_screen_start_time) <
            2 * kScreenTimeUsageUpdateFrequency) {
      current_screen_start_time_ = current_screen_start_time;
    } else {
      // If kCurrentScreenStartTime is not set or it's been too long since the
      // last update, set the time to now.
      current_screen_start_time_ = now;
    }
    pref_service_->SetTime(prefs::kCurrentScreenStartTime,
                           current_screen_start_time_);
    pref_service_->CommitPendingWrite();
    CheckTimeLimit();

    save_screen_time_timer_.Start(
        FROM_HERE, kScreenTimeUsageUpdateFrequency,
        base::BindRepeating(
            &ScreenTimeController::SaveScreenTimeProgressPeriodically,
            base::Unretained(this)));
  }
}

}  // namespace chromeos
