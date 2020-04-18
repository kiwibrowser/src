// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/keyboard_brightness/tray_keyboard_brightness.h"

#include <memory>

#include "ash/shell.h"
#include "ash/system/tray/system_tray_item.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ui/views/view.h"

namespace ash {

class TrayKeyboardBrightnessTest : public AshTestBase {
 public:
  TrayKeyboardBrightnessTest() = default;
  ~TrayKeyboardBrightnessTest() override = default;

 protected:
  views::View* CreateDetailedView() {
    TrayKeyboardBrightness tray(nullptr);
    // The login status doesn't matter here; supply any random value.
    return tray.CreateDetailedView(LoginStatus::USER);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TrayKeyboardBrightnessTest);
};

// Tests that when the detailed view is initially created that its
// KeyboardBrightnessView is created as visible.
TEST_F(TrayKeyboardBrightnessTest, CreateDetailedView) {
  std::unique_ptr<views::View> tray(CreateDetailedView());
  EXPECT_TRUE(tray->visible());
}

// Tests that when the detailed view is created during TabletMode that its
// KeyboardBrightnessView is visible.
TEST_F(TrayKeyboardBrightnessTest, CreateDetailedViewDuringTabletMode) {
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  std::unique_ptr<views::View> tray(CreateDetailedView());
  EXPECT_TRUE(tray->visible());
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
}

// Tests that the enabling of TabletMode has no affect on the visibility of a
// previously created KeyboardBrightnessView that belongs to a detailed view.
TEST_F(TrayKeyboardBrightnessTest,
       DetailedViewVisibilityChangesDuringTabletMode) {
  std::unique_ptr<views::View> tray(CreateDetailedView());
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  EXPECT_TRUE(tray->visible());
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  EXPECT_TRUE(tray->visible());
}

}  // namespace ash
