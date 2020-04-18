// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/night_light/tray_night_light.h"

#include "ash/public/cpp/ash_switches.h"
#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/system/night_light/night_light_controller.h"
#include "ash/system/tray/system_tray.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ash/test_shell_delegate.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "components/prefs/testing_pref_service.h"

namespace ash {

namespace {

constexpr char kFakeUserEmail[] = "fake_user@nightlight";

class TrayNightLightTest : public NoSessionAshTestBase {
 public:
  TrayNightLightTest() = default;
  ~TrayNightLightTest() override = default;

  // AshTestBase:
  void SetUp() override {
    // Explicitly enable the NightLight feature for the tests.
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        ash::switches::kAshEnableNightLight);

    NoSessionAshTestBase::SetUp();
    SimulateUserLogin(kFakeUserEmail);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TrayNightLightTest);
};

// Tests that when NightLight is active, its tray icon in the System Tray is
// visible.
TEST_F(TrayNightLightTest, TestNightLightTrayVisibility) {
  SystemTray* tray = GetPrimarySystemTray();
  TrayNightLight* tray_night_light = tray->tray_night_light();
  NightLightController* controller = Shell::Get()->night_light_controller();
  ASSERT_FALSE(controller->GetEnabled());
  controller->Toggle();
  ASSERT_TRUE(controller->GetEnabled());
  EXPECT_TRUE(tray_night_light->tray_view()->visible());
  controller->Toggle();
  ASSERT_FALSE(controller->GetEnabled());
  EXPECT_FALSE(tray_night_light->tray_view()->visible());
  controller->Toggle();
  ASSERT_TRUE(controller->GetEnabled());
  EXPECT_TRUE(tray_night_light->tray_view()->visible());
}

}  // namespace

}  // namespace ash
