// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/arc_migration_guide_notification.h"

#include <memory>

#include "ash/public/cpp/vector_icons/vector_icons.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/arc/arc_migration_constants.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager_client.h"
#include "components/account_id/account_id.h"
#include "components/arc/arc_prefs.h"
#include "components/user_manager/known_user.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/chromeos/devicetype_utils.h"
#include "ui/gfx/color_palette.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_delegate.h"

namespace arc {

namespace {

constexpr char kNotifierId[] = "arc_fs_migration";
constexpr char kSuggestNotificationId[] = "arc_fs_migration/suggest";
constexpr char kSuccessNotificationId[] = "arc_fs_migration/success";
constexpr base::TimeDelta kSuccessNotificationDelay =
    base::TimeDelta::FromSeconds(3);

void DoShowArcMigrationSuccessNotification(Profile* profile) {
  message_center::NotifierId notifier_id(
      message_center::NotifierId::SYSTEM_COMPONENT, kNotifierId);
  notifier_id.profile_id =
      multi_user_util::GetAccountIdFromProfile(profile).GetUserEmail();

  auto delegate =
      base::MakeRefCounted<message_center::HandleNotificationClickDelegate>(
          base::BindRepeating(
              [](Profile* profile) {
                arc::SetArcPlayStoreEnabledForProfile(profile, true);
              },
              profile));

  std::unique_ptr<message_center::Notification> notification =
      message_center::Notification::CreateSystemNotification(
          message_center::NOTIFICATION_TYPE_SIMPLE, kSuccessNotificationId,
          l10n_util::GetStringUTF16(IDS_ARC_MIGRATE_ENCRYPTION_SUCCESS_TITLE),
          l10n_util::GetStringUTF16(IDS_ARC_MIGRATE_ENCRYPTION_SUCCESS_MESSAGE),
          gfx::Image(), base::string16(), GURL(), notifier_id,
          message_center::RichNotificationData(), std::move(delegate),
          ash::kNotificationSettingsIcon,
          message_center::SystemNotificationWarningLevel::NORMAL);

  NotificationDisplayService::GetForProfile(profile)->Display(
      NotificationHandler::Type::TRANSIENT, *notification);
}

}  // namespace

// static
void ShowArcMigrationGuideNotification(Profile* profile) {
  message_center::NotifierId notifier_id(
      message_center::NotifierId::SYSTEM_COMPONENT, kNotifierId);
  notifier_id.profile_id =
      multi_user_util::GetAccountIdFromProfile(profile).GetUserEmail();

  base::Optional<power_manager::PowerSupplyProperties> power =
      chromeos::DBusThreadManager::Get()
          ->GetPowerManagerClient()
          ->GetLastStatus();
  const bool is_low_battery =
      power &&
      power->battery_state() !=
          power_manager::PowerSupplyProperties_BatteryState_NOT_PRESENT &&
      power->battery_percent() < kMigrationMinimumBatteryPercent;

  const base::string16 message = ui::SubstituteChromeOSDeviceType(
      is_low_battery
          ? IDS_ARC_MIGRATE_ENCRYPTION_NOTIFICATION_LOW_BATTERY_MESSAGE
          : IDS_ARC_MIGRATE_ENCRYPTION_NOTIFICATION_MESSAGE);

  auto delegate =
      base::MakeRefCounted<message_center::HandleNotificationClickDelegate>(
          base::BindRepeating(&chrome::AttemptUserExit));

  std::unique_ptr<message_center::Notification> notification =
      message_center::Notification::CreateSystemNotification(
          message_center::NOTIFICATION_TYPE_SIMPLE, kSuggestNotificationId,
          l10n_util::GetStringUTF16(
              IDS_ARC_MIGRATE_ENCRYPTION_NOTIFICATION_TITLE),
          message, gfx::Image(), base::string16(), GURL(), notifier_id,
          message_center::RichNotificationData(), std::move(delegate),
          ash::kNotificationSettingsIcon,
          message_center::SystemNotificationWarningLevel::CRITICAL_WARNING);
  notification->set_renotify(true);

  NotificationDisplayService::GetForProfile(profile)->Display(
      NotificationHandler::Type::TRANSIENT, *notification);
}

void ShowArcMigrationSuccessNotificationIfNeeded(Profile* profile) {
  const AccountId account_id =
      multi_user_util::GetAccountIdFromProfile(profile);

  int pref_value = kFileSystemIncompatible;
  user_manager::known_user::GetIntegerPref(
      account_id, prefs::kArcCompatibleFilesystemChosen, &pref_value);

  // Show notification only when the pref value indicates the file system is
  // compatible, but not yet notified.
  if (pref_value != kFileSystemCompatible)
    return;

  if (profile->IsNewProfile()) {
    // If this is a newly created profile, the filesystem was compatible from
    // the beginning, not because of migration. Skip showing the notification.
  } else if (!arc::IsArcAllowedForProfile(profile)) {
    // TODO(kinaba; crbug.com/721631): the current message mentions ARC,
    // which is inappropriate for users disallowed running ARC.

    // Log a warning message because, for now, this should not basically happen
    // except for some exceptional situation or due to some bug.
    LOG(WARNING) << "Migration has happened for an ARC-disallowed user.";
  } else {
    // Delay the notification to make sure that it is not hidden behind windows
    // which are shown at the beginning of user session (e.g. Chrome).
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&DoShowArcMigrationSuccessNotification, profile),
        kSuccessNotificationDelay);
  }

  // Mark as notified.
  user_manager::known_user::SetIntegerPref(
      account_id, prefs::kArcCompatibleFilesystemChosen,
      arc::kFileSystemCompatibleAndNotified);
}

}  // namespace arc
