// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/night_light/night_light_feature_pod_controller.h"

#include "ash/public/cpp/ash_switches.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/night_light/night_light_controller.h"
#include "ash/system/unified/feature_pod_button.h"
#include "ui/base/l10n/l10n_util.h"

namespace ash {

NightLightFeaturePodController::NightLightFeaturePodController() {}

NightLightFeaturePodController::~NightLightFeaturePodController() = default;

FeaturePodButton* NightLightFeaturePodController::CreateButton() {
  DCHECK(!button_);
  button_ = new FeaturePodButton(this);
  button_->SetVisible(switches::IsNightLightEnabled());
  button_->SetLabel(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_NIGHT_LIGHT_BUTTON_LABEL));
  UpdateButton();
  return button_;
}

void NightLightFeaturePodController::OnIconPressed() {
  DCHECK(switches::IsNightLightEnabled());
  Shell::Get()->night_light_controller()->Toggle();
  UpdateButton();
}

void NightLightFeaturePodController::UpdateButton() {
  if (!switches::IsNightLightEnabled())
    return;

  bool is_enabled = Shell::Get()->night_light_controller()->GetEnabled();
  button_->SetToggled(is_enabled);
  button_->SetVectorIcon(is_enabled ? kSystemMenuNightLightOnIcon
                                    : kSystemMenuNightLightOffIcon);
}

}  // namespace ash
