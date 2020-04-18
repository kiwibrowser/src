// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tiles/tray_tiles.h"

#include "ash/public/cpp/ash_switches.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/system/night_light/night_light_controller.h"
#include "ash/system/night_light/night_light_toggle_button.h"
#include "ash/system/tiles/tiles_default_view.h"
#include "ash/system/tray/system_menu_button.h"
#include "ash/test/ash_test_base.h"
#include "base/command_line.h"
#include "components/user_manager/user_type.h"
#include "ui/views/view.h"

using views::Button;

namespace ash {

// Tests manually control their session state.
class TrayTilesTest : public NoSessionAshTestBase {
 public:
  TrayTilesTest() = default;
  ~TrayTilesTest() override = default;

  void SetUp() override {
    // Explicitly enable the NightLight feature for the tests.
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        ash::switches::kAshEnableNightLight);

    NoSessionAshTestBase::SetUp();
    tray_tiles_.reset(new TrayTiles(GetPrimarySystemTray()));
  }

  void TearDown() override {
    tray_tiles_.reset();
    NoSessionAshTestBase::TearDown();
  }

  views::Button* GetSettingsButton() {
    return tray_tiles()->default_view_->settings_button_;
  }

  views::Button* GetHelpButton() {
    return tray_tiles()->default_view_->help_button_;
  }

  NightLightToggleButton* GetNightLightButton() {
    return tray_tiles()->default_view_->night_light_button_;
  }

  views::Button* GetLockButton() {
    return tray_tiles()->default_view_->lock_button_;
  }

  views::Button* GetPowerButton() {
    return tray_tiles()->default_view_->power_button_;
  }

  TrayTiles* tray_tiles() { return tray_tiles_.get(); }

 private:
  std::unique_ptr<TrayTiles> tray_tiles_;

  DISALLOW_COPY_AND_ASSIGN(TrayTilesTest);
};

// Settings buttons are disabled before login.
TEST_F(TrayTilesTest, ButtonStatesNotLoggedIn) {
  std::unique_ptr<views::View> default_view(
      tray_tiles()->CreateDefaultViewForTesting());
  EXPECT_EQ(Button::STATE_DISABLED, GetSettingsButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetHelpButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetNightLightButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetLockButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetPowerButton()->state());
}

// All buttons are enabled after login.
TEST_F(TrayTilesTest, ButtonStatesLoggedIn) {
  CreateUserSessions(1);
  std::unique_ptr<views::View> default_view(
      tray_tiles()->CreateDefaultViewForTesting());
  EXPECT_EQ(Button::STATE_NORMAL, GetSettingsButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetHelpButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetNightLightButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetLockButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetPowerButton()->state());
}

// Settings buttons are disabled at the lock screen.
TEST_F(TrayTilesTest, ButtonStatesLockScreen) {
  BlockUserSession(BLOCKED_BY_LOCK_SCREEN);
  std::unique_ptr<views::View> default_view(
      tray_tiles()->CreateDefaultViewForTesting());
  EXPECT_EQ(Button::STATE_DISABLED, GetSettingsButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetHelpButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetNightLightButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetLockButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetPowerButton()->state());
}

// Settings buttons are disabled when adding a second multiprofile user.
TEST_F(TrayTilesTest, ButtonStatesAddingUser) {
  SetUserAddingScreenRunning(true);
  std::unique_ptr<views::View> default_view(
      tray_tiles()->CreateDefaultViewForTesting());
  EXPECT_EQ(Button::STATE_DISABLED, GetSettingsButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetHelpButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetNightLightButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetLockButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetPowerButton()->state());
}

// Settings buttons are disabled when adding a supervised user.
TEST_F(TrayTilesTest, ButtonStatesSupervisedUserFlow) {
  // Simulate the add supervised user flow, which is a regular user session but
  // with web UI settings disabled.
  const bool enable_settings = false;
  GetSessionControllerClient()->AddUserSession(
      "foo@example.com", user_manager::USER_TYPE_REGULAR, enable_settings);
  std::unique_ptr<views::View> default_view(
      tray_tiles()->CreateDefaultViewForTesting());
  EXPECT_EQ(Button::STATE_DISABLED, GetSettingsButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetHelpButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetNightLightButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetLockButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetPowerButton()->state());
}

}  // namespace ash
