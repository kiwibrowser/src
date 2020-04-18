// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/message_center/arc_notification_manager_delegate_impl.h"

#include "ash/login_status.h"
#include "ash/message_center/message_center_controller.h"
#include "ash/public/cpp/ash_features.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/system/message_center/notification_tray.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/unified/unified_system_tray.h"

namespace ash {

ArcNotificationManagerDelegateImpl::ArcNotificationManagerDelegateImpl() =
    default;
ArcNotificationManagerDelegateImpl::~ArcNotificationManagerDelegateImpl() =
    default;

bool ArcNotificationManagerDelegateImpl::IsPublicSessionOrKiosk() const {
  const LoginStatus login_status =
      Shell::Get()->session_controller()->login_status();

  return login_status == LoginStatus::PUBLIC ||
         login_status == LoginStatus::KIOSK_APP ||
         login_status == LoginStatus::ARC_KIOSK_APP;
}

void ArcNotificationManagerDelegateImpl::GetAppIdByPackageName(
    const std::string& package_name,
    GetAppIdByPackageNameCallback callback) {
  if (package_name.empty()) {
    std::move(callback).Run(std::string());
    return;
  }

  Shell::Get()->message_center_controller()->GetArcAppIdByPackageName(
      package_name, std::move(callback));
}

void ArcNotificationManagerDelegateImpl::ShowMessageCenter() {
  if (features::IsSystemTrayUnifiedEnabled()) {
    Shell::Get()
        ->GetPrimaryRootWindowController()
        ->GetStatusAreaWidget()
        ->unified_system_tray()
        ->ShowBubble(false /* show_by_click */);
  } else {
    Shell::Get()->GetNotificationTray()->ShowMessageCenter(
        false /* show_by_click */);
  }
}

}  // namespace ash
