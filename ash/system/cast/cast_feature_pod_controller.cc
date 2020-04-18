// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/cast/cast_feature_pod_controller.h"

#include "ash/cast_config_controller.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/unified/feature_pod_button.h"
#include "ash/system/unified/unified_system_tray_controller.h"
#include "ui/base/l10n/l10n_util.h"

namespace ash {

CastFeaturePodController::CastFeaturePodController(
    UnifiedSystemTrayController* tray_controller)
    : tray_controller_(tray_controller) {}

CastFeaturePodController::~CastFeaturePodController() = default;

FeaturePodButton* CastFeaturePodController::CreateButton() {
  button_ = new FeaturePodButton(this);
  button_->SetVectorIcon(kSystemMenuCastIcon);
  button_->SetLabel(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_CAST_DESKTOP));
  CastConfigController* cast_config = Shell::Get()->cast_config();
  button_->SetVisible(cast_config->Connected() &&
                      cast_config->HasSinksAndRoutes() &&
                      !cast_config->HasActiveRoute());
  return button_;
}

void CastFeaturePodController::OnIconPressed() {
  tray_controller_->ShowCastDetailedView();
}

}  // namespace ash
