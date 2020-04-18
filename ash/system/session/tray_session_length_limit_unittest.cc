// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/session/tray_session_length_limit.h"

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/system/tray/label_tray_view.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_test_api.h"
#include "ash/test/ash_test_base.h"
#include "base/time/time.h"

namespace ash {

class TraySessionLengthLimitTest : public AshTestBase {
 public:
  TraySessionLengthLimitTest() = default;
  ~TraySessionLengthLimitTest() override = default;

 protected:
  LabelTrayView* GetSessionLengthLimitTrayView() {
    return SystemTrayTestApi(GetPrimarySystemTray())
        .tray_session_length_limit()
        ->tray_bubble_view_;
  }

  void UpdateSessionLengthLimitInMin(int mins) {
    Shell::Get()->session_controller()->SetSessionLengthLimit(
        base::TimeDelta::FromMinutes(mins), base::TimeTicks::Now());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TraySessionLengthLimitTest);
};

TEST_F(TraySessionLengthLimitTest, Visibility) {
  SystemTray* system_tray = GetPrimarySystemTray();

  // By default there is no session length limit item.
  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_FALSE(GetSessionLengthLimitTrayView());
  system_tray->CloseBubble();

  // Setting a length limit shows an item in the system tray menu.
  UpdateSessionLengthLimitInMin(10);
  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  ASSERT_TRUE(GetSessionLengthLimitTrayView());
  EXPECT_TRUE(GetSessionLengthLimitTrayView()->visible());
  system_tray->CloseBubble();

  // Removing the session length limit removes the tray menu item.
  UpdateSessionLengthLimitInMin(0);
  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_FALSE(GetSessionLengthLimitTrayView());
  system_tray->CloseBubble();
}

}  // namespace ash
