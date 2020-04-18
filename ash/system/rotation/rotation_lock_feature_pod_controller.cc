// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/rotation/rotation_lock_feature_pod_controller.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/unified/feature_pod_button.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ui/base/l10n/l10n_util.h"

namespace ash {

RotationLockFeaturePodController::RotationLockFeaturePodController() {
  DCHECK(Shell::Get());
  Shell::Get()->tablet_mode_controller()->AddObserver(this);
  Shell::Get()->screen_orientation_controller()->AddObserver(this);
}

RotationLockFeaturePodController::~RotationLockFeaturePodController() {
  if (Shell::Get()->screen_orientation_controller())
    Shell::Get()->screen_orientation_controller()->RemoveObserver(this);
  if (Shell::Get()->tablet_mode_controller())
    Shell::Get()->tablet_mode_controller()->RemoveObserver(this);
}

FeaturePodButton* RotationLockFeaturePodController::CreateButton() {
  DCHECK(!button_);
  button_ = new FeaturePodButton(this);
  button_->SetLabel(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_ROTATION_LOCK));
  UpdateButton();
  return button_;
}

void RotationLockFeaturePodController::OnIconPressed() {
  Shell::Get()->screen_orientation_controller()->ToggleUserRotationLock();
}

void RotationLockFeaturePodController::OnTabletModeStarted() {
  UpdateButton();
}

void RotationLockFeaturePodController::OnTabletModeEnded() {
  UpdateButton();
}

void RotationLockFeaturePodController::OnUserRotationLockChanged() {
  UpdateButton();
}

void RotationLockFeaturePodController::UpdateButton() {
  bool tablet_enabled = Shell::Get()
                            ->tablet_mode_controller()
                            ->IsTabletModeWindowManagerEnabled();

  button_->SetVisible(tablet_enabled);

  if (!tablet_enabled)
    return;

  bool rotation_locked =
      Shell::Get()->screen_orientation_controller()->user_rotation_locked();
  bool is_portrait = Shell::Get()
                         ->screen_orientation_controller()
                         ->IsUserLockedOrientationPortrait();

  button_->SetToggled(rotation_locked);

  if (rotation_locked && is_portrait) {
    button_->SetVectorIcon(kSystemMenuRotationLockPortraitIcon);
    button_->SetSubLabel(
        l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_ROTATION_LOCK_PORTRAIT));
  } else if (rotation_locked && !is_portrait) {
    button_->SetVectorIcon(kSystemMenuRotationLockLandscapeIcon);
    button_->SetSubLabel(
        l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_ROTATION_LOCK_LANDSCAPE));
  } else {
    button_->SetVectorIcon(kSystemMenuRotationLockAutoIcon);
    button_->SetSubLabel(
        l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_ROTATION_LOCK_AUTO));
  }
}

}  // namespace ash
