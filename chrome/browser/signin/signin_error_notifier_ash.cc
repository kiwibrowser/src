// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/signin_error_notifier_ash.h"

#include "ash/public/cpp/vector_icons/vector_icons.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/chromeos/login/user_flow.h"
#include "chrome/browser/chromeos/login/users/chrome_user_manager.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/notifications/notification_common.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_pages.h"
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

const char kProfileSigninNotificationId[] = "chrome://settings/signin/";

void HandleNotificationClick(base::Optional<int> button_index) {
  chrome::AttemptUserExit();
}

}  // namespace

SigninErrorNotifier::SigninErrorNotifier(SigninErrorController* controller,
                                         Profile* profile)
    : error_controller_(controller),
      profile_(profile) {
  // Create a unique notification ID for this profile.
  notification_id_ =
      kProfileSigninNotificationId + profile->GetProfileUserName();

  error_controller_->AddObserver(this);
  OnErrorChanged();
}

SigninErrorNotifier::~SigninErrorNotifier() {
  DCHECK(!error_controller_)
      << "SigninErrorNotifier::Shutdown() was not called";
}

void SigninErrorNotifier::Shutdown() {
  error_controller_->RemoveObserver(this);
  error_controller_ = NULL;
}

void SigninErrorNotifier::OnErrorChanged() {
  if (!error_controller_->HasError()) {
    NotificationDisplayService::GetForProfile(profile_)->Close(
        NotificationHandler::Type::TRANSIENT, notification_id_);
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

  // Add an accept button to sign the user out.
  message_center::RichNotificationData data;
  data.buttons.push_back(message_center::ButtonInfo(
      l10n_util::GetStringUTF16(IDS_SYNC_RELOGIN_LINK_LABEL)));

  message_center::NotifierId notifier_id(
      message_center::NotifierId::SYSTEM_COMPONENT,
      kProfileSigninNotificationId);

  // Set |profile_id| for multi-user notification blocker.
  notifier_id.profile_id =
      multi_user_util::GetAccountIdFromProfile(profile_).GetUserEmail();

  std::unique_ptr<message_center::Notification> notification =
      message_center::Notification::CreateSystemNotification(
          message_center::NOTIFICATION_TYPE_SIMPLE, notification_id_,
          l10n_util::GetStringUTF16(IDS_SIGNIN_ERROR_BUBBLE_VIEW_TITLE),
          GetMessageBody(), gfx::Image(),
          l10n_util::GetStringUTF16(IDS_SIGNIN_ERROR_DISPLAY_SOURCE),
          GURL(notification_id_), notifier_id, data,
          new message_center::HandleNotificationClickDelegate(
              base::Bind(&HandleNotificationClick)),
          ash::kNotificationWarningIcon,
          message_center::SystemNotificationWarningLevel::WARNING);
  notification->SetSystemPriority();

  // Update or add the notification.
  NotificationDisplayService::GetForProfile(profile_)->Display(
      NotificationHandler::Type::TRANSIENT, *notification);
}

base::string16 SigninErrorNotifier::GetMessageBody() const {
  switch (error_controller_->auth_error().state()) {
    // TODO(rogerta): use account id in error messages.

    // User credentials are invalid (bad acct, etc).
    case GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS:
    case GoogleServiceAuthError::SERVICE_ERROR:
    case GoogleServiceAuthError::ACCOUNT_DELETED:
    case GoogleServiceAuthError::ACCOUNT_DISABLED:
      return l10n_util::GetStringUTF16(
          IDS_SYNC_SIGN_IN_ERROR_BUBBLE_VIEW_MESSAGE);
      break;

    // Sync service is not available for this account's domain.
    case GoogleServiceAuthError::SERVICE_UNAVAILABLE:
      return l10n_util::GetStringUTF16(
          IDS_SYNC_UNAVAILABLE_ERROR_BUBBLE_VIEW_MESSAGE);
      break;

    // Generic message for "other" errors.
    default:
      return l10n_util::GetStringUTF16(
          IDS_SYNC_OTHER_SIGN_IN_ERROR_BUBBLE_VIEW_MESSAGE);
  }
}
