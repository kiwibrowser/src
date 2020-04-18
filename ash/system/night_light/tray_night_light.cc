// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/night_light/tray_night_light.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ui/views/view.h"

namespace ash {

TrayNightLight::TrayNightLight(SystemTray* system_tray)
    : TrayImageItem(system_tray, kSystemTrayNightLightIcon, UMA_NIGHT_LIGHT) {
  Shell::Get()->night_light_controller()->AddObserver(this);
}

TrayNightLight::~TrayNightLight() {
  Shell::Get()->night_light_controller()->RemoveObserver(this);
}

void TrayNightLight::OnNightLightEnabledChanged(bool enabled) {
  if (tray_view())
    tray_view()->SetVisible(enabled);
}

bool TrayNightLight::GetInitialVisibility() {
  return Shell::Get()->night_light_controller()->GetEnabled();
}

}  // namespace ash
