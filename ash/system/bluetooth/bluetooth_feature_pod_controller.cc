// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/bluetooth/bluetooth_feature_pod_controller.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/bluetooth/tray_bluetooth_helper.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/system/unified/feature_pod_button.h"
#include "ash/system/unified/unified_system_tray_controller.h"
#include "ui/base/l10n/l10n_util.h"

namespace ash {

BluetoothFeaturePodController::BluetoothFeaturePodController(
    UnifiedSystemTrayController* tray_controller)
    : tray_controller_(tray_controller) {
  Shell::Get()->system_tray_notifier()->AddBluetoothObserver(this);
}

BluetoothFeaturePodController::~BluetoothFeaturePodController() {
  Shell::Get()->system_tray_notifier()->RemoveBluetoothObserver(this);
}

FeaturePodButton* BluetoothFeaturePodController::CreateButton() {
  DCHECK(!button_);
  button_ = new FeaturePodButton(this);
  button_->SetVectorIcon(kSystemMenuBluetoothIcon);
  UpdateButton();
  return button_;
}

void BluetoothFeaturePodController::OnIconPressed() {
  Shell::Get()->tray_bluetooth_helper()->SetBluetoothEnabled(
      !button_->IsToggled());
}

void BluetoothFeaturePodController::OnLabelPressed() {
  Shell::Get()->tray_bluetooth_helper()->SetBluetoothEnabled(true);
  tray_controller_->ShowBluetoothDetailedView();
}

void BluetoothFeaturePodController::UpdateButton() {
  bool is_available =
      Shell::Get()->tray_bluetooth_helper()->GetBluetoothAvailable();
  button_->SetVisible(is_available);
  if (!is_available)
    return;

  bool is_enabled =
      Shell::Get()->tray_bluetooth_helper()->GetBluetoothEnabled();
  button_->SetToggled(is_enabled);
  button_->SetLabel(l10n_util::GetStringUTF16(
      is_enabled ? IDS_ASH_STATUS_TRAY_BLUETOOTH_ENABLED
                 : IDS_ASH_STATUS_TRAY_BLUETOOTH_DISABLED));
}

void BluetoothFeaturePodController::OnBluetoothRefresh() {
  UpdateButton();
}

void BluetoothFeaturePodController::OnBluetoothDiscoveringChanged() {
  UpdateButton();
}

}  // namespace ash
