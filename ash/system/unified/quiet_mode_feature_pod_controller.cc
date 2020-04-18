// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/quiet_mode_feature_pod_controller.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/unified/feature_pod_button.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/message_center.h"

using message_center::MessageCenter;

namespace ash {

QuietModeFeaturePodController::QuietModeFeaturePodController() {
  MessageCenter::Get()->AddObserver(this);
}

QuietModeFeaturePodController::~QuietModeFeaturePodController() {
  MessageCenter::Get()->RemoveObserver(this);
}

FeaturePodButton* QuietModeFeaturePodController::CreateButton() {
  DCHECK(!button_);
  button_ = new FeaturePodButton(this);
  button_->SetLabel(l10n_util::GetStringUTF16(
      IDS_ASH_MESSAGE_CENTER_QUIET_MODE_BUTTON_TOOLTIP));
  OnQuietModeChanged(MessageCenter::Get()->IsQuietMode());
  return button_;
}

void QuietModeFeaturePodController::OnIconPressed() {
  MessageCenter* message_center = MessageCenter::Get();
  bool is_quiet_mode = message_center->IsQuietMode();
  message_center->SetQuietMode(!is_quiet_mode);
}

void QuietModeFeaturePodController::OnQuietModeChanged(bool in_quiet_mode) {
  button_->SetVectorIcon(in_quiet_mode
                             ? kNotificationCenterDoNotDisturbOnIcon
                             : kNotificationCenterDoNotDisturbOffIcon);
  button_->SetToggled(in_quiet_mode);
}

}  // namespace ash
