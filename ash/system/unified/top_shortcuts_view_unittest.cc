// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/top_shortcuts_view.h"

#include "ash/session/test_session_controller_client.h"
#include "ash/system/unified/collapse_button.h"
#include "ash/system/unified/sign_out_button.h"
#include "ash/system/unified/top_shortcut_button.h"
#include "ash/system/unified/unified_system_tray_controller.h"
#include "ash/system/unified/unified_system_tray_model.h"
#include "ash/test/ash_test_base.h"

using views::Button;

namespace ash {

// Tests manually control their session state.
class TopShortcutsViewTest : public NoSessionAshTestBase {
 public:
  TopShortcutsViewTest() = default;
  ~TopShortcutsViewTest() override = default;

  void SetUp() override {
    NoSessionAshTestBase::SetUp();

    model_ = std::make_unique<UnifiedSystemTrayModel>();
    controller_ = std::make_unique<UnifiedSystemTrayController>(
        model_.get(), GetPrimarySystemTray());
  }

  void TearDown() override {
    controller_.reset();
    top_shortcuts_view_.reset();
    model_.reset();
    NoSessionAshTestBase::TearDown();
  }

 protected:
  void SetUpView() {
    top_shortcuts_view_ = std::make_unique<TopShortcutsView>(controller_.get());
  }

  views::View* GetUserAvatar() {
    return top_shortcuts_view_->user_avatar_button_;
  }

  views::Button* GetSignOutButton() {
    return top_shortcuts_view_->sign_out_button_;
  }

  views::Button* GetLockButton() { return top_shortcuts_view_->lock_button_; }

  views::Button* GetSettingsButton() {
    return top_shortcuts_view_->settings_button_;
  }

  views::Button* GetPowerButton() { return top_shortcuts_view_->power_button_; }

  views::Button* GetCollapseButton() {
    return top_shortcuts_view_->collapse_button_;
  }

 private:
  std::unique_ptr<UnifiedSystemTrayModel> model_;
  std::unique_ptr<UnifiedSystemTrayController> controller_;
  std::unique_ptr<TopShortcutsView> top_shortcuts_view_;

  DISALLOW_COPY_AND_ASSIGN(TopShortcutsViewTest);
};

// Settings buttons are disabled before login.
TEST_F(TopShortcutsViewTest, ButtonStatesNotLoggedIn) {
  SetUpView();
  EXPECT_EQ(nullptr, GetUserAvatar());
  EXPECT_FALSE(GetSignOutButton()->visible());
  EXPECT_EQ(Button::STATE_DISABLED, GetLockButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetSettingsButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetPowerButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetCollapseButton()->state());
}

// All buttons are enabled after login.
TEST_F(TopShortcutsViewTest, ButtonStatesLoggedIn) {
  CreateUserSessions(1);
  SetUpView();
  EXPECT_NE(nullptr, GetUserAvatar());
  EXPECT_TRUE(GetSignOutButton()->visible());
  EXPECT_EQ(Button::STATE_NORMAL, GetLockButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetSettingsButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetPowerButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetCollapseButton()->state());
}

// Settings buttons are disabled at the lock screen.
TEST_F(TopShortcutsViewTest, ButtonStatesLockScreen) {
  BlockUserSession(BLOCKED_BY_LOCK_SCREEN);
  SetUpView();
  EXPECT_NE(nullptr, GetUserAvatar());
  EXPECT_TRUE(GetSignOutButton()->visible());
  EXPECT_EQ(Button::STATE_DISABLED, GetLockButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetSettingsButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetPowerButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetCollapseButton()->state());
}

// Settings buttons are disabled when adding a second multiprofile user.
TEST_F(TopShortcutsViewTest, ButtonStatesAddingUser) {
  CreateUserSessions(1);
  SetUserAddingScreenRunning(true);
  SetUpView();
  EXPECT_NE(nullptr, GetUserAvatar());
  EXPECT_TRUE(GetSignOutButton()->visible());
  EXPECT_EQ(Button::STATE_DISABLED, GetLockButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetSettingsButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetPowerButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetCollapseButton()->state());
}

// Settings buttons are disabled when adding a supervised user.
TEST_F(TopShortcutsViewTest, ButtonStatesSupervisedUserFlow) {
  // Simulate the add supervised user flow, which is a regular user session but
  // with web UI settings disabled.
  const bool enable_settings = false;
  GetSessionControllerClient()->AddUserSession(
      "foo@example.com", user_manager::USER_TYPE_REGULAR, enable_settings);
  SetUpView();
  EXPECT_EQ(nullptr, GetUserAvatar());
  EXPECT_FALSE(GetSignOutButton()->visible());
  EXPECT_EQ(Button::STATE_DISABLED, GetLockButton()->state());
  EXPECT_EQ(Button::STATE_DISABLED, GetSettingsButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetPowerButton()->state());
  EXPECT_EQ(Button::STATE_NORMAL, GetCollapseButton()->state());
}

}  // namespace ash
