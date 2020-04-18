// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/sync_error_notifier_ash.h"

#include "ash/public/cpp/vector_icons/vector_icons.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/user_flow.h"
#include "chrome/browser/chromeos/login/users/chrome_user_manager.h"
#include "chrome/browser/notifications/notification_common.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/webui/signin/login_ui_service.h"
#include "chrome/browser/ui/webui/signin/login_ui_service_factory.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/account_id/account_id.h"
#include "components/user_manager/user_manager.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_delegate.h"

namespace {

const char kProfileSyncNotificationId[] = "chrome://settings/sync/";

void ShowSyncSetup(Profile* profile) {
  LoginUIService* login_ui = LoginUIServiceFactory::GetForProfile(profile);
  if (login_ui->current_login_ui()) {
    // TODO(michaelpg): The LoginUI might be on an inactive desktop.
    // See crbug.com/354280.
    login_ui->current_login_ui()->FocusUI();
    return;
  }

  chrome::ShowSettingsSubPageForProfile(profile, chrome::kSyncSetupSubPage);
}

}  // namespace

SyncErrorNotifier::SyncErrorNotifier(syncer::SyncErrorController* controller,
                                     Profile* profile)
    : error_controller_(controller),
      profile_(profile),
      notification_displayed_(false) {
  // Create a unique notification ID for this profile.
  notification_id_ =
      kProfileSyncNotificationId + profile_->GetProfileUserName();

  error_controller_->AddObserver(this);
  OnErrorChanged();
}

SyncErrorNotifier::~SyncErrorNotifier() {
  DCHECK(!error_controller_)
      << "SyncErrorNotifier::Shutdown() was not called";
}

void SyncErrorNotifier::Shutdown() {
  error_controller_->RemoveObserver(this);
  error_controller_ = nullptr;
}

void SyncErrorNotifier::OnErrorChanged() {
  if (error_controller_->HasError() == notification_displayed_)
    return;

  auto* display_service = NotificationDisplayService::GetForProfile(profile_);
  if (!error_controller_->HasError()) {
    notification_displayed_ = false;
    display_service->Close(NotificationHandler::Type::TRANSIENT,
                           notification_id_);
    return;
  }

  if (user_manager::UserManager::IsInitialized()) {
    chromeos::UserFlow* user_flow =
        chromeos::ChromeUserManager::Get()->GetCurrentUserFlow();

    // Check whether Chrome OS user flow allows launching browser.
    // Example: Supervised user creation flow which handles token invalidation
    // itself and notifications should be suppressed. http://crbug.com/359045
    if (!user_flow->ShouldLaunchBrowser())
      return;
  }

  // Error state just got triggered. There shouldn't be previous notification.
  // Let's display one.
  DCHECK(!notification_displayed_ && error_controller_->HasError());

  message_center::NotifierId notifier_id(
      message_center::NotifierId::SYSTEM_COMPONENT, kProfileSyncNotificationId);

  // Set |profile_id| for multi-user notification blocker.
  notifier_id.profile_id =
      multi_user_util::GetAccountIdFromProfile(profile_).GetUserEmail();

  // Add a new notification.
  std::unique_ptr<message_center::Notification> notification =
      message_center::Notification::CreateSystemNotification(
          message_center::NOTIFICATION_TYPE_SIMPLE, notification_id_,
          l10n_util::GetStringUTF16(IDS_SYNC_ERROR_BUBBLE_VIEW_TITLE),
          l10n_util::GetStringUTF16(
              IDS_SYNC_PASSPHRASE_ERROR_BUBBLE_VIEW_MESSAGE),
          gfx::Image(),
          l10n_util::GetStringUTF16(IDS_SIGNIN_ERROR_DISPLAY_SOURCE),
          GURL(notification_id_), notifier_id,
          message_center::RichNotificationData(),
          base::MakeRefCounted<message_center::HandleNotificationClickDelegate>(
              base::BindRepeating(&ShowSyncSetup, profile_)),
          ash::kNotificationWarningIcon,
          message_center::SystemNotificationWarningLevel::WARNING);

  display_service->Display(NotificationHandler::Type::TRANSIENT, *notification);
  notification_displayed_ = true;
}
