// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/quick_unlock/quick_unlock_notification_controller.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/quick_unlock/quick_unlock_factory.h"
#include "chrome/browser/chromeos/login/quick_unlock/quick_unlock_storage.h"
#include "chrome/browser/chromeos/login/quick_unlock/quick_unlock_utils.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/message_center/public/cpp/notification.h"

namespace chromeos {
namespace quick_unlock {
namespace {

constexpr char kPinNotificationId[] = "pinunlock_notification";
constexpr char kPinSetupUrl[] = "chrome://settings/lockScreen";
constexpr char kNotifierPinUnlock[] = "ash.pinunlock";
constexpr char kFingerprintNotificationId[] = "fingerprintunlock_notification";
constexpr char kFingerprintSetupUrl[] =
    "chrome://settings/lockScreen/fingerprint";
const char kNotifierFingerprintUnlock[] = "ash.fingerprintunlock";

}  // namespace

QuickUnlockNotificationController::QuickUnlockNotificationController(
    Profile* profile)
    : profile_(profile) {
  registrar_.Add(this, chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED,
                 content::NotificationService::AllSources());
}

QuickUnlockNotificationController::~QuickUnlockNotificationController() {
  UnregisterObserver();
}

// static
QuickUnlockNotificationController*
QuickUnlockNotificationController::CreateForPin(Profile* profile) {
  QuickUnlockNotificationController* controller =
      new QuickUnlockNotificationController(profile);

  // Set the PIN notification parameters.
  NotificationParams* params = &controller->params_;
  params->title_message_id = IDS_QUICK_UNLOCK_NOTIFICATION_TITLE;
  params->body_message_id = IDS_QUICK_UNLOCK_NOTIFICATION_BODY;
  // TODO(http://crbug.com/291747): Change this to actual icon for quick unlock
  // feature notification, also use a vector icon instead of raster asset.
  params->icon_id = IDR_SCREENSHOT_NOTIFICATION_ICON;
  params->notifier = kNotifierPinUnlock;
  params->feature_name_id = IDS_PIN_UNLOCK_FEATURE_NOTIFIER_NAME;
  params->notification_id = kPinNotificationId;
  params->url = GURL(kPinSetupUrl);
  params->was_shown_pref_id = prefs::kPinUnlockFeatureNotificationShown;

  controller->should_show_notification_callback_ =
      base::Bind(&QuickUnlockNotificationController::ShouldShowPinNotification);

  return controller;
}

// static
bool QuickUnlockNotificationController::ShouldShowPinNotification(
    Profile* profile) {
  // Do not show notification if this is a guest session.
  if (profile->IsGuestSession())
    return false;

  // Do not show notification to user if already displayed in the past.
  if (profile->GetPrefs()->GetBoolean(
          prefs::kPinUnlockFeatureNotificationShown)) {
    return false;
  }

  // Do not show notification if policy does not allow PIN, or if user is
  // supervised.
  if (IsPinDisabledByPolicy(profile->GetPrefs()) ||
      !IsPinEnabled(profile->GetPrefs())) {
    return false;
  }

  // TODO(jdufault): Enable once quick unlock settings land. See
  // https://crbug.com/826773. Do not show the notification if the PIN is
  // already set.
  return false;
}

// static
QuickUnlockNotificationController*
QuickUnlockNotificationController::CreateForFingerprint(Profile* profile) {
  QuickUnlockNotificationController* controller =
      new QuickUnlockNotificationController(profile);

  // Set the fingerprint notification parameters.
  NotificationParams* params = &controller->params_;
  params->title_message_id = IDS_FINGERPRINT_NOTIFICATION_TITLE;
  params->body_message_id = IDS_FINGERPRINT_NOTIFICATION_BODY;
  // TODO(sammiequon): Change to a vector icon identifier.
  params->icon_id = IDR_NOTIFICATION_FINGERPRINT;
  params->notifier = kNotifierFingerprintUnlock;
  params->feature_name_id = IDS_FINGERPRINT_UNLOCK_FEATURE_NOTIFIER_NAME;
  params->notification_id = kFingerprintNotificationId;
  params->url = GURL(kFingerprintSetupUrl);
  params->was_shown_pref_id = prefs::kFingerprintUnlockFeatureNotificationShown;

  controller->should_show_notification_callback_ = base::Bind(
      &QuickUnlockNotificationController::ShouldShowFingerprintNotification);

  return controller;
}

// static
bool QuickUnlockNotificationController::ShouldShowFingerprintNotification(
    Profile* profile) {
  // Do not show notification if this is a guest session.
  if (profile->IsGuestSession())
    return false;

  // Do not show notification to user if already displayed in the past.
  if (profile->GetPrefs()->GetBoolean(
          prefs::kFingerprintUnlockFeatureNotificationShown)) {
    return false;
  }

  if (!IsFingerprintEnabled())
    return false;

  // TODO(sammiequon): Enable once fingerprint api is ready. Do not enable
  // before http://crbug.com/695639 is resolved.
  return false;
}

// NotificationDelegate override:
void QuickUnlockNotificationController::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  if (type != chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED)
    return;

  bool is_screen_locked = *content::Details<bool>(details).ptr();

  // Return if the screen is locked, indicating that the notification was
  // emitted due to a screen lock event.
  if (is_screen_locked)
    return;

  UnregisterObserver();

  // The user may have enabled the quick unlock feature during the current
  // session and after the notificaiton controller has already been initialized.
  DCHECK(!should_show_notification_callback_.is_null());
  if (should_show_notification_callback_.Run(profile_)) {
    SetNotificationPreferenceWasShown();
    return;
  }

  std::unique_ptr<message_center::Notification> notification =
      CreateNotification();
  NotificationDisplayService::GetForProfile(profile_)->Display(
      NotificationHandler::Type::TRANSIENT, *notification);
}

// message_center::NotificationDelegate override:
void QuickUnlockNotificationController::Close(bool by_user) {
  if (by_user)
    SetNotificationPreferenceWasShown();
}

// message_center::NotificationDelegate override:
void QuickUnlockNotificationController::Click(
    const base::Optional<int>& button_index,
    const base::Optional<base::string16>& reply) {
  NavigateParams params(profile_, params_.url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.window_action = NavigateParams::SHOW_WINDOW;
  Navigate(&params);

  SetNotificationPreferenceWasShown();

  // Remove the notification from tray.
  NotificationDisplayService::GetForProfile(profile_)->Close(
      NotificationHandler::Type::TRANSIENT, params_.notification_id);
}

void QuickUnlockNotificationController::SetNotificationPreferenceWasShown() {
  PrefService* pref_service = profile_->GetPrefs();
  pref_service->SetBoolean(params_.was_shown_pref_id, true);
}

void QuickUnlockNotificationController::UnregisterObserver() {
  if (registrar_.IsRegistered(this,
                              chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED,
                              content::NotificationService::AllSources())) {
    registrar_.Remove(this, chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED,
                      content::NotificationService::AllSources());
  }
}

std::unique_ptr<message_center::Notification>
QuickUnlockNotificationController::CreateNotification() {
  return std::make_unique<message_center::Notification>(
      message_center::NOTIFICATION_TYPE_SIMPLE, params_.notification_id,
      l10n_util::GetStringUTF16(params_.title_message_id),
      l10n_util::GetStringUTF16(params_.body_message_id),
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(params_.icon_id),
      l10n_util::GetStringUTF16(params_.feature_name_id), GURL(),
      message_center::NotifierId(message_center::NotifierId::SYSTEM_COMPONENT,
                                 params_.notifier),
      message_center::RichNotificationData(), this);
}

QuickUnlockNotificationController::NotificationParams::NotificationParams() {}

QuickUnlockNotificationController::NotificationParams::~NotificationParams() {}

}  // namespace quick_unlock
}  // namespace chromeos
