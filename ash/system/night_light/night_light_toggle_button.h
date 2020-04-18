// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_NIGHT_LIGHT_NIGHT_LIGHT_TOGGLE_BUTTON_H_
#define ASH_SYSTEM_NIGHT_LIGHT_NIGHT_LIGHT_TOGGLE_BUTTON_H_

#include "ash/system/tray/system_menu_button.h"
#include "base/macros.h"

namespace ash {

// The NightLight toggle button in the system tray.
class NightLightToggleButton : public SystemMenuButton {
 public:
  explicit NightLightToggleButton(views::ButtonListener* listener);
  ~NightLightToggleButton() override = default;

  // Toggles the status of NightLight.
  void Toggle();

 private:
  // Updates the icon and its style based on the status of NightLight.
  void Update();

  // views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  DISALLOW_COPY_AND_ASSIGN(NightLightToggleButton);
};

}  // namespace ash

#endif  // ASH_SYSTEM_NIGHT_LIGHT_NIGHT_LIGHT_TOGGLE_BUTTON_H_
