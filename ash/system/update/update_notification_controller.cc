// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/update/update_notification_controller.h"

#include "ash/public/cpp/ash_features.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/model/system_tray_model.h"
#include "ash/system/tray/system_tray_controller.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/notification.h"

using message_center::MessageCenter;
using message_center::Notification;

namespace ash {

namespace {

const char kNotifierId[] = "ash.update";

}  // namespace

// static
const char UpdateNotificationController::kNotificationId[] = "chrome://update";

UpdateNotificationController::UpdateNotificationController()
    : model_(Shell::Get()->system_tray_model()->update_model()) {
  DCHECK(features::IsSystemTrayUnifiedEnabled());
  model_->AddObserver(this);
  OnUpdateAvailable();
}

UpdateNotificationController::~UpdateNotificationController() {
  model_->RemoveObserver(this);
}

void UpdateNotificationController::OnUpdateAvailable() {
  if (!ShouldShowUpdate()) {
    message_center::MessageCenter::Get()->RemoveNotification(
        kNotificationId, false /* by_user */);
    return;
  }

  // TODO(tetsui): Update resource strings according to UX.
  std::unique_ptr<Notification> notification =
      Notification::CreateSystemNotification(
          message_center::NOTIFICATION_TYPE_SIMPLE, kNotificationId,
          GetNotificationTitle(), base::string16() /* message */, gfx::Image(),
          base::string16() /* display_source */, GURL(),
          message_center::NotifierId(
              message_center::NotifierId::SYSTEM_COMPONENT, kNotifierId),
          message_center::RichNotificationData(),
          base::MakeRefCounted<message_center::HandleNotificationClickDelegate>(
              base::BindRepeating(
                  &UpdateNotificationController::HandleNotificationClick,
                  weak_ptr_factory_.GetWeakPtr())),
          kSystemMenuUpdateIcon,
          message_center::SystemNotificationWarningLevel::NORMAL);
  notification->set_pinned(true);
  MessageCenter::Get()->AddNotification(std::move(notification));
}

bool UpdateNotificationController::ShouldShowUpdate() const {
  return model_->update_required() || model_->update_over_cellular_available();
}

base::string16 UpdateNotificationController::GetNotificationTitle() const {
  // TODO(tetsui): Update resource strings according to UX.
  if (model_->factory_reset_required()) {
    return l10n_util::GetStringUTF16(
        IDS_ASH_STATUS_TRAY_RESTART_AND_POWERWASH_TO_UPDATE);
  }
  if (model_->update_type() == mojom::UpdateType::FLASH) {
    return l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_UPDATE_FLASH);
  }

  if (!model_->update_required() && model_->update_over_cellular_available()) {
    return l10n_util::GetStringUTF16(
        IDS_ASH_STATUS_TRAY_UPDATE_OVER_CELLULAR_AVAILABLE);
  }

  return l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_UPDATE);
}

void UpdateNotificationController::HandleNotificationClick() {
  DCHECK(ShouldShowUpdate());

  message_center::MessageCenter::Get()->RemoveNotification(kNotificationId,
                                                           false /* by_user */);

  if (model_->update_required()) {
    Shell::Get()->system_tray_controller()->RequestRestartForUpdate();
    Shell::Get()->metrics()->RecordUserMetricsAction(
        UMA_STATUS_AREA_OS_UPDATE_DEFAULT_SELECTED);
  } else {
    // Shows the about chrome OS page and checks for update after the page is
    // loaded.
    Shell::Get()->system_tray_controller()->ShowAboutChromeOS();
  }
}

}  // namespace ash
