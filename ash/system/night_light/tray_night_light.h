// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_NIGHT_LIGHT_TRAY_NIGHT_LIGHT_H_
#define ASH_SYSTEM_NIGHT_LIGHT_TRAY_NIGHT_LIGHT_H_

#include "ash/system/night_light/night_light_controller.h"
#include "ash/system/tray/tray_image_item.h"
#include "base/macros.h"

namespace ash {

// Shows the NightLight icon in the system tray whenever NightLight is enabled
// and active.
class TrayNightLight : public TrayImageItem,
                       public NightLightController::Observer {
 public:
  explicit TrayNightLight(SystemTray* system_tray);
  ~TrayNightLight() override;

  // ash::NightLightController::Observer:
  void OnNightLightEnabledChanged(bool enabled) override;

  // ash::TrayImageItem:
  bool GetInitialVisibility() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TrayNightLight);
};

}  // namespace ash

#endif  // ASH_SYSTEM_NIGHT_LIGHT_TRAY_NIGHT_LIGHT_H_
