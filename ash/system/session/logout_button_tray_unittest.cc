// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/session/logout_button_tray.h"

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/config.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shell.h"
#include "ash/system/status_area_widget.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ash/test_shell_delegate.h"
#include "base/macros.h"
#include "components/prefs/pref_service.h"

namespace ash {
namespace {

constexpr char kUserEmail[] = "user1@test.com";

class LogoutButtonTrayTest : public NoSessionAshTestBase {
 public:
  LogoutButtonTrayTest() = default;
  ~LogoutButtonTrayTest() override = default;

  // NoSessionAshTestBase:
  void SetUp() override {
    NoSessionAshTestBase::SetUp();
    SimulateUserLogin(kUserEmail);
  }

  PrefService* pref_service() {
    return Shell::Get()->session_controller()->GetUserPrefServiceForUser(
        AccountId::FromUserEmail(kUserEmail));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(LogoutButtonTrayTest);
};

TEST_F(LogoutButtonTrayTest, Visibility) {
  // Button is not visible before login.
  LogoutButtonTray* button = Shell::GetPrimaryRootWindowController()
                                 ->GetStatusAreaWidget()
                                 ->logout_button_tray_for_testing();
  ASSERT_TRUE(button);
  EXPECT_FALSE(button->visible());

  // Button is not visible after simulated login.
  EXPECT_FALSE(button->visible());

  // Setting the pref makes the button visible.
  pref_service()->SetBoolean(prefs::kShowLogoutButtonInTray, true);
  EXPECT_TRUE(button->visible());

  // Locking the screen hides the button.
  TestSessionControllerClient* const session = GetSessionControllerClient();
  session->RequestLockScreen();
  EXPECT_FALSE(button->visible());

  // Unlocking the screen shows the button.
  session->UnlockScreen();
  EXPECT_TRUE(button->visible());

  // Resetting the pref hides the button.
  pref_service()->SetBoolean(prefs::kShowLogoutButtonInTray, false);
  EXPECT_FALSE(button->visible());
}

}  // namespace
}  // namespace ash
