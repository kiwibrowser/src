// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/locale/locale_notification_controller.h"

#include <memory>
#include <utility>

#include "ash/public/cpp/vector_icons/vector_icons.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "base/strings/string16.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_delegate.h"
#include "ui/message_center/public/cpp/notification_types.h"

using message_center::Notification;

namespace ash {
namespace {

const char kLocaleChangeNotificationId[] = "chrome://settings/locale";
const char kNotifierLocale[] = "ash.locale";

class LocaleNotificationDelegate : public message_center::NotificationDelegate {
 public:
  explicit LocaleNotificationDelegate(
      base::OnceCallback<void(ash::mojom::LocaleNotificationResult)> callback);

 protected:
  ~LocaleNotificationDelegate() override;

  // message_center::NotificationDelegate overrides:
  void Close(bool by_user) override;
  void Click(const base::Optional<int>& button_index,
             const base::Optional<base::string16>& reply) override;

 private:
  base::OnceCallback<void(ash::mojom::LocaleNotificationResult)> callback_;

  DISALLOW_COPY_AND_ASSIGN(LocaleNotificationDelegate);
};

LocaleNotificationDelegate::LocaleNotificationDelegate(
    base::OnceCallback<void(ash::mojom::LocaleNotificationResult)> callback)
    : callback_(std::move(callback)) {}

LocaleNotificationDelegate::~LocaleNotificationDelegate() {
  if (callback_) {
    // We're being destroyed but the user didn't click on anything. Run the
    // callback so that we don't crash.
    std::move(callback_).Run(ash::mojom::LocaleNotificationResult::ACCEPT);
  }
}

void LocaleNotificationDelegate::Close(bool by_user) {
  if (callback_) {
    std::move(callback_).Run(ash::mojom::LocaleNotificationResult::ACCEPT);
  }
}

void LocaleNotificationDelegate::Click(
    const base::Optional<int>& button_index,
    const base::Optional<base::string16>& reply) {
  if (!callback_)
    return;

  std::move(callback_).Run(button_index
                               ? ash::mojom::LocaleNotificationResult::REVERT
                               : ash::mojom::LocaleNotificationResult::ACCEPT);
}

}  // namespace

LocaleNotificationController::LocaleNotificationController() = default;

LocaleNotificationController::~LocaleNotificationController() = default;

void LocaleNotificationController::BindRequest(
    mojom::LocaleNotificationControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void LocaleNotificationController::OnLocaleChanged(
    const std::string& cur_locale,
    const std::string& from_locale,
    const std::string& to_locale,
    OnLocaleChangedCallback callback) {
  base::string16 from =
      l10n_util::GetDisplayNameForLocale(from_locale, cur_locale, true);
  base::string16 to =
      l10n_util::GetDisplayNameForLocale(to_locale, cur_locale, true);

  message_center::RichNotificationData optional;
  optional.buttons.push_back(
      message_center::ButtonInfo(l10n_util::GetStringFUTF16(
          IDS_ASH_STATUS_TRAY_LOCALE_REVERT_MESSAGE, from)));
  optional.never_timeout = true;

  std::unique_ptr<Notification> notification =
      Notification::CreateSystemNotification(
          message_center::NOTIFICATION_TYPE_SIMPLE, kLocaleChangeNotificationId,
          l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_LOCALE_CHANGE_TITLE),
          l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_LOCALE_CHANGE_MESSAGE,
                                     from, to),
          gfx::Image(), base::string16() /* display_source */, GURL(),
          message_center::NotifierId(
              message_center::NotifierId::SYSTEM_COMPONENT, kNotifierLocale),
          optional, new LocaleNotificationDelegate(std::move(callback)),
          kNotificationSettingsIcon,
          message_center::SystemNotificationWarningLevel::NORMAL);
  message_center::MessageCenter::Get()->AddNotification(
      std::move(notification));
}

}  // namespace ash
