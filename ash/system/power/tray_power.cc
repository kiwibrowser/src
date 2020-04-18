// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/tray_power.h"

#include <utility>

#include "ash/accessibility/accessibility_delegate.h"
#include "ash/public/cpp/ash_switches.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/date/date_view.h"
#include "ash/system/power/battery_notification.h"
#include "ash/system/power/dual_role_notification.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_item_view.h"
#include "ash/system/tray/tray_utils.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/time/time.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/chromeos/devicetype_utils.h"
#include "ui/gfx/image/image_skia_source.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_delegate.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/view.h"

using message_center::MessageCenter;
using message_center::Notification;

namespace ash {

namespace tray {

PowerTrayView::PowerTrayView(SystemTrayItem* owner) : TrayItemView(owner) {
  CreateImageView();
  UpdateImage();
  UpdateStatus();
  PowerStatus::Get()->AddObserver(this);
}

PowerTrayView::~PowerTrayView() {
  PowerStatus::Get()->RemoveObserver(this);
}

void PowerTrayView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->SetName(accessible_name_);
  node_data->role = ax::mojom::Role::kButton;
}

void PowerTrayView::OnPowerStatusChanged() {
  UpdateStatus();
}

void PowerTrayView::UpdateStatus() {
  UpdateImage();
  SetVisible(PowerStatus::Get()->IsBatteryPresent());
  accessible_name_ = PowerStatus::Get()->GetAccessibleNameString(true);
}

void PowerTrayView::UpdateImage() {
  const PowerStatus::BatteryImageInfo& info =
      PowerStatus::Get()->GetBatteryImageInfo();
  // Only change the image when the info changes. http://crbug.com/589348
  if (info_ && info_->ApproximatelyEqual(info))
    return;
  info_ = info;
  image_view()->SetImage(PowerStatus::GetBatteryImage(
      info, kTrayIconSize, SkColorSetA(kTrayIconColor, 0x4C), kTrayIconColor));
}

}  // namespace tray

TrayPower::TrayPower(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_POWER) {
}

TrayPower::~TrayPower() = default;

views::View* TrayPower::CreateTrayView(LoginStatus status) {
  // There may not be enough information when this is created about whether
  // there is a battery or not. So always create this, and adjust visibility as
  // necessary.
  CHECK(power_tray_ == nullptr);
  power_tray_ = new tray::PowerTrayView(this);
  return power_tray_;
}

views::View* TrayPower::CreateDefaultView(LoginStatus status) {
  // Make sure icon status is up to date. (Also triggers stub activation).
  PowerStatus::Get()->RequestStatusUpdate();
  return nullptr;
}

void TrayPower::OnTrayViewDestroyed() {
  power_tray_ = nullptr;
}

}  // namespace ash
