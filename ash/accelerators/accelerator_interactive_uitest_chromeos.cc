// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/accelerator_controller.h"

#include "ash/app_list/test/app_list_test_helper.h"
#include "ash/shell.h"
#include "ash/shell_observer.h"
#include "ash/system/network/network_observer.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/test/ash_interactive_ui_test_base.h"
#include "ash/test_screenshot_delegate.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "base/run_loop.h"
#include "base/test/user_action_tester.h"
#include "chromeos/network/network_handler.h"
#include "ui/base/test/ui_controls.h"

namespace ash {

namespace {

// A network observer to watch for the toggle wifi events.
class TestNetworkObserver : public NetworkObserver {
 public:
  TestNetworkObserver() : wifi_enabled_status_(false) {}

  // ash::NetworkObserver:
  void RequestToggleWifi() override {
    wifi_enabled_status_ = !wifi_enabled_status_;
  }

  bool wifi_enabled_status() const { return wifi_enabled_status_; }

 private:
  bool wifi_enabled_status_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkObserver);
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////

// This is intended to test few samples from each category of accelerators to
// make sure they work properly. The test is done as an interactive ui test
// using ui_controls::Send*() functions.
// This is to catch any future regressions (crbug.com/469235).
class AcceleratorInteractiveUITest : public AshInteractiveUITestBase,
                                     public ShellObserver {
 public:
  AcceleratorInteractiveUITest() : is_in_overview_mode_(false) {}

  void SetUp() override {
    AshInteractiveUITestBase::SetUp();

    Shell::Get()->AddShellObserver(this);

    chromeos::NetworkHandler::Initialize();
  }

  void TearDown() override {
    chromeos::NetworkHandler::Shutdown();

    Shell::Get()->RemoveShellObserver(this);

    AshInteractiveUITestBase::TearDown();
  }

  // Sends a key press event and waits synchronously until it's completely
  // processed.
  void SendKeyPressSync(ui::KeyboardCode key,
                        bool control,
                        bool shift,
                        bool alt) {
    base::RunLoop loop;
    ui_controls::SendKeyPressNotifyWhenDone(Shell::GetPrimaryRootWindow(), key,
                                            control, shift, alt, false,
                                            loop.QuitClosure());
    loop.Run();
  }

  // ash::ShellObserver:
  void OnOverviewModeStarting() override { is_in_overview_mode_ = true; }
  void OnOverviewModeEnded() override { is_in_overview_mode_ = false; }

 protected:
  bool is_in_overview_mode_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AcceleratorInteractiveUITest);
};

////////////////////////////////////////////////////////////////////////////////

#if defined(OFFICIAL_BUILD)
#define MAYBE_NonRepeatableNeedingWindowActions \
  DISABLED_NonRepeatableNeedingWindowActions
#define MAYBE_ChromeOsAccelerators DISABLED_ChromeOsAccelerators
#define MAYBE_ToggleAppList DISABLED_ToggleAppList
#else
#define MAYBE_NonRepeatableNeedingWindowActions \
  NonRepeatableNeedingWindowActions
#define MAYBE_ChromeOsAccelerators ChromeOsAccelerators
#define MAYBE_ToggleAppList ToggleAppList
#endif  // defined(OFFICIAL_BUILD)

// Tests a sample of the non-repeatable accelerators that need windows to be
// enabled.
TEST_F(AcceleratorInteractiveUITest, MAYBE_NonRepeatableNeedingWindowActions) {
  // Create a bunch of windows to work with.
  aura::Window* window_1 =
      CreateTestWindowInShellWithBounds(gfx::Rect(0, 0, 100, 100));
  aura::Window* window_2 =
      CreateTestWindowInShellWithBounds(gfx::Rect(0, 0, 100, 100));
  window_1->Show();
  wm::ActivateWindow(window_1);
  window_2->Show();
  wm::ActivateWindow(window_2);

  // Test TOGGLE_OVERVIEW.
  EXPECT_FALSE(is_in_overview_mode_);
  SendKeyPressSync(ui::VKEY_MEDIA_LAUNCH_APP1, false, false, false);
  EXPECT_TRUE(is_in_overview_mode_);
  SendKeyPressSync(ui::VKEY_MEDIA_LAUNCH_APP1, false, false, false);
  EXPECT_FALSE(is_in_overview_mode_);

  // Test CYCLE_FORWARD_MRU and CYCLE_BACKWARD_MRU.
  wm::ActivateWindow(window_1);
  EXPECT_TRUE(wm::IsActiveWindow(window_1));
  EXPECT_FALSE(wm::IsActiveWindow(window_2));
  SendKeyPressSync(ui::VKEY_TAB, false, false, true);  // CYCLE_FORWARD_MRU.
  EXPECT_TRUE(wm::IsActiveWindow(window_2));
  EXPECT_FALSE(wm::IsActiveWindow(window_1));
  SendKeyPressSync(ui::VKEY_TAB, false, true, true);  // CYCLE_BACKWARD_MRU.
  EXPECT_TRUE(wm::IsActiveWindow(window_1));
  EXPECT_FALSE(wm::IsActiveWindow(window_2));

  // Test TOGGLE_FULLSCREEN.
  wm::WindowState* active_window_state = wm::GetActiveWindowState();
  EXPECT_FALSE(active_window_state->IsFullscreen());
  SendKeyPressSync(ui::VKEY_MEDIA_LAUNCH_APP2, false, false, false);
  EXPECT_TRUE(active_window_state->IsFullscreen());
  SendKeyPressSync(ui::VKEY_MEDIA_LAUNCH_APP2, false, false, false);
  EXPECT_FALSE(active_window_state->IsFullscreen());
}

// Tests a sample of ChromeOS specific accelerators.
TEST_F(AcceleratorInteractiveUITest, MAYBE_ChromeOsAccelerators) {
  // Test TAKE_SCREENSHOT and TAKE_PARTIAL_SCREENSHOT.
  TestScreenshotDelegate* screenshot_delegate = GetScreenshotDelegate();
  screenshot_delegate->set_can_take_screenshot(true);
  EXPECT_EQ(0, screenshot_delegate->handle_take_screenshot_count());
  SendKeyPressSync(ui::VKEY_MEDIA_LAUNCH_APP1, true, false, false);
  EXPECT_EQ(1, screenshot_delegate->handle_take_screenshot_count());
  SendKeyPressSync(ui::VKEY_PRINT, false, false, false);
  EXPECT_EQ(2, screenshot_delegate->handle_take_screenshot_count());
  SendKeyPressSync(ui::VKEY_MEDIA_LAUNCH_APP1, true, true, false);
  EXPECT_EQ(2, screenshot_delegate->handle_take_screenshot_count());
  // Press ESC to go out of the partial screenshot mode.
  SendKeyPressSync(ui::VKEY_ESCAPE, false, false, false);

  // Test VOLUME_MUTE.
  base::UserActionTester user_action_tester;
  EXPECT_EQ(0, user_action_tester.GetActionCount("Accel_VolumeMute_F8"));
  SendKeyPressSync(ui::VKEY_VOLUME_MUTE, false, false, false);
  EXPECT_EQ(1, user_action_tester.GetActionCount("Accel_VolumeMute_F8"));
  // Test VOLUME_DOWN.
  EXPECT_EQ(0, user_action_tester.GetActionCount("Accel_VolumeDown_F9"));
  SendKeyPressSync(ui::VKEY_VOLUME_DOWN, false, false, false);
  EXPECT_EQ(1, user_action_tester.GetActionCount("Accel_VolumeDown_F9"));
  // Test VOLUME_UP.
  EXPECT_EQ(0, user_action_tester.GetActionCount("Accel_VolumeUp_F10"));
  SendKeyPressSync(ui::VKEY_VOLUME_UP, false, false, false);
  EXPECT_EQ(1, user_action_tester.GetActionCount("Accel_VolumeUp_F10"));

  // Test TOGGLE_WIFI.
  TestNetworkObserver network_observer;
  Shell::Get()->system_tray_notifier()->AddNetworkObserver(&network_observer);

  EXPECT_FALSE(network_observer.wifi_enabled_status());
  SendKeyPressSync(ui::VKEY_WLAN, false, false, false);
  EXPECT_TRUE(network_observer.wifi_enabled_status());
  SendKeyPressSync(ui::VKEY_WLAN, false, false, false);
  EXPECT_FALSE(network_observer.wifi_enabled_status());

  Shell::Get()->system_tray_notifier()->RemoveNetworkObserver(
      &network_observer);
}

// Tests the app list accelerator.
TEST_F(AcceleratorInteractiveUITest, MAYBE_ToggleAppList) {
  GetAppListTestHelper()->CheckVisibility(false);
  SendKeyPressSync(ui::VKEY_LWIN, false, false, false);
  RunAllPendingInMessageLoop();
  GetAppListTestHelper()->CheckVisibility(true);
  SendKeyPressSync(ui::VKEY_LWIN, false, false, false);
  RunAllPendingInMessageLoop();
  GetAppListTestHelper()->CheckVisibility(false);
}

}  // namespace ash
