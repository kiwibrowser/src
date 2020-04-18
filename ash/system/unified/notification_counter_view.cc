// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/notification_counter_view.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_utils.h"
#include "base/i18n/number_formatting.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/message_center/message_center.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"

namespace ash {

NotificationCounterView::NotificationCounterView() : TrayItemView(nullptr) {
  CreateLabel();
  Update();
}

NotificationCounterView::~NotificationCounterView() = default;

void NotificationCounterView::Update() {
  size_t notification_count =
      message_center::MessageCenter::Get()->NotificationCount();
  SetupLabelForTray(label());
  label()->SetText(base::FormatNumber(notification_count));
  SetVisible(notification_count > 0 &&
             !message_center::MessageCenter::Get()->IsQuietMode());
}

QuietModeView::QuietModeView() : TrayItemView(nullptr) {
  CreateImageView();
  image_view()->SetImage(gfx::CreateVectorIcon(
      kNotificationCenterDoNotDisturbOnIcon, kTrayIconSize, kTrayIconColor));
  Update();
}

QuietModeView::~QuietModeView() = default;

void QuietModeView::Update() {
  SetVisible(message_center::MessageCenter::Get()->IsQuietMode());
}

}  // namespace ash
