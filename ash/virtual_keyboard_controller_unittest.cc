// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/virtual_keyboard_controller.h"

#include <utility>
#include <vector>

#include "ash/accessibility/accessibility_controller.h"
#include "ash/ime/ime_controller.h"
#include "ash/ime/test_ime_controller_client.h"
#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/system/virtual_keyboard/virtual_keyboard_observer.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/tablet_mode/scoped_disable_internal_mouse_and_keyboard.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/command_line.h"
#include "services/ui/public/cpp/input_devices/input_device_client_test_api.h"
#include "ui/events/devices/input_device.h"
#include "ui/events/devices/touchscreen_device.h"
#include "ui/keyboard/keyboard_switches.h"
#include "ui/keyboard/keyboard_util.h"

namespace ash {

class VirtualKeyboardControllerTest : public AshTestBase {
 public:
  VirtualKeyboardControllerTest() = default;
  ~VirtualKeyboardControllerTest() override = default;

  // Sets the event blocker on the maximized window controller.
  void SetEventBlocker(
      std::unique_ptr<ScopedDisableInternalMouseAndKeyboard> blocker) {
    Shell::Get()->tablet_mode_controller()->event_blocker_ = std::move(blocker);
  }

  void SetUp() override {
    AshTestBase::SetUp();
    ui::InputDeviceClientTestApi().SetKeyboardDevices({});
    ui::InputDeviceClientTestApi().SetTouchscreenDevices({});
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(VirtualKeyboardControllerTest);
};

// Mock event blocker that enables the internal keyboard when it's destructor
// is called.
class MockEventBlocker : public ScopedDisableInternalMouseAndKeyboard {
 public:
  MockEventBlocker() = default;
  ~MockEventBlocker() override {
    std::vector<ui::InputDevice> keyboard_devices;
    keyboard_devices.push_back(ui::InputDevice(
        1, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, "keyboard"));
    ui::InputDeviceClientTestApi().SetKeyboardDevices(keyboard_devices);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockEventBlocker);
};

// Tests that reenabling keyboard devices while shutting down does not
// cause the Virtual Keyboard Controller to crash. See crbug.com/446204.
TEST_F(VirtualKeyboardControllerTest, RestoreKeyboardDevices) {
  // Toggle tablet mode on.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  std::unique_ptr<ScopedDisableInternalMouseAndKeyboard> blocker(
      new MockEventBlocker);
  SetEventBlocker(std::move(blocker));
}

TEST_F(VirtualKeyboardControllerTest,
       ForceToShowKeyboardWithKeysetWhenAccessibilityKeyboardIsEnabled) {
  // TODO(mash): Turning on accessibility keyboard does not create a valid
  // KeyboardController under MASH. See https://crbug.com/646565.
  if (Shell::GetAshConfig() == Config::MASH)
    return;

  AccessibilityController* accessibility_controller =
      Shell::Get()->accessibility_controller();
  accessibility_controller->SetVirtualKeyboardEnabled(true);
  ASSERT_TRUE(accessibility_controller->IsVirtualKeyboardEnabled());

  // Set up a mock ImeControllerClient to test keyset changes.
  TestImeControllerClient client;
  Shell::Get()->ime_controller()->SetClient(client.CreateInterfacePtr());

  // Should show the keyboard without messing with accessibility prefs.
  Shell::Get()->virtual_keyboard_controller()->ForceShowKeyboardWithKeyset(
      chromeos::input_method::mojom::ImeKeyset::kEmoji);
  Shell::Get()->ime_controller()->FlushMojoForTesting();
  EXPECT_TRUE(accessibility_controller->IsVirtualKeyboardEnabled());

  // Keyset should be emoji.
  Shell::Get()->ime_controller()->FlushMojoForTesting();
  EXPECT_EQ(chromeos::input_method::mojom::ImeKeyset::kEmoji,
            client.last_keyset_);

  // Simulate the keyboard hiding.
  if (keyboard::KeyboardController::GetInstance()->HasObserver(
          Shell::Get()->virtual_keyboard_controller())) {
    Shell::Get()->virtual_keyboard_controller()->OnKeyboardHidden();
  }
  base::RunLoop().RunUntilIdle();

  // The keyboard should still be enabled.
  EXPECT_TRUE(accessibility_controller->IsVirtualKeyboardEnabled());

  // Reset the accessibility prefs.
  accessibility_controller->SetVirtualKeyboardEnabled(false);

  // Keyset should be reset to none.
  Shell::Get()->ime_controller()->FlushMojoForTesting();
  EXPECT_EQ(chromeos::input_method::mojom::ImeKeyset::kNone,
            client.last_keyset_);
}

TEST_F(VirtualKeyboardControllerTest,
       ForceToShowKeyboardWithKeysetWhenAccessibilityKeyboardIsDisabled) {
  // TODO(mash): Turning on accessibility keyboard does not create a valid
  // KeyboardController under MASH. See https://crbug.com/646565.
  if (Shell::GetAshConfig() == Config::MASH)
    return;

  AccessibilityController* accessibility_controller =
      Shell::Get()->accessibility_controller();
  accessibility_controller->SetVirtualKeyboardEnabled(false);
  ASSERT_FALSE(accessibility_controller->IsVirtualKeyboardEnabled());

  // Set up a mock ImeControllerClient to test keyset changes.
  TestImeControllerClient client;
  Shell::Get()->ime_controller()->SetClient(client.CreateInterfacePtr());

  // Should show the keyboard by turning on the accesibility keyboard.
  Shell::Get()->virtual_keyboard_controller()->ForceShowKeyboardWithKeyset(
      chromeos::input_method::mojom::ImeKeyset::kEmoji);
  Shell::Get()->ime_controller()->FlushMojoForTesting();
  EXPECT_TRUE(accessibility_controller->IsVirtualKeyboardEnabled());

  // Keyset should be emoji.
  EXPECT_EQ(chromeos::input_method::mojom::ImeKeyset::kEmoji,
            client.last_keyset_);

  // Simulate the keyboard hiding.
  if (keyboard::KeyboardController::GetInstance()->HasObserver(
          Shell::Get()->virtual_keyboard_controller())) {
    Shell::Get()->virtual_keyboard_controller()->OnKeyboardHidden();
  }
  base::RunLoop().RunUntilIdle();

  // The keyboard should still be disabled again.
  EXPECT_FALSE(accessibility_controller->IsVirtualKeyboardEnabled());

  // Keyset should be reset to none.
  Shell::Get()->ime_controller()->FlushMojoForTesting();
  EXPECT_EQ(chromeos::input_method::mojom::ImeKeyset::kNone,
            client.last_keyset_);
}

class VirtualKeyboardControllerAutoTest : public VirtualKeyboardControllerTest,
                                          public VirtualKeyboardObserver {
 public:
  VirtualKeyboardControllerAutoTest() : notified_(false), suppressed_(false) {}
  ~VirtualKeyboardControllerAutoTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();
    // Set the current list of devices to empty so that they don't interfere
    // with the test.
    ui::InputDeviceClientTestApi().SetKeyboardDevices({});
    ui::InputDeviceClientTestApi().SetTouchscreenDevices({});
    Shell::Get()->system_tray_notifier()->AddVirtualKeyboardObserver(this);
  }

  void TearDown() override {
    Shell::Get()->system_tray_notifier()->RemoveVirtualKeyboardObserver(this);
    AshTestBase::TearDown();
  }

  void OnKeyboardSuppressionChanged(bool suppressed) override {
    notified_ = true;
    suppressed_ = suppressed;
  }

  void ResetObserver() {
    suppressed_ = false;
    notified_ = false;
  }

  bool IsVirtualKeyboardSuppressed() { return suppressed_; }

  bool notified() { return notified_; }

 private:
  // Whether the observer method was called.
  bool notified_;

  // Whether the keeyboard is suppressed.
  bool suppressed_;

  DISALLOW_COPY_AND_ASSIGN(VirtualKeyboardControllerAutoTest);
};

// Tests that the onscreen keyboard is disabled if an internal keyboard is
// present and tablet mode is disabled.
TEST_F(VirtualKeyboardControllerAutoTest, DisabledIfInternalKeyboardPresent) {
  std::vector<ui::TouchscreenDevice> screens;
  screens.push_back(
      ui::TouchscreenDevice(1, ui::InputDeviceType::INPUT_DEVICE_INTERNAL,
                            "Touchscreen", gfx::Size(1024, 768), 0));
  ui::InputDeviceClientTestApi().SetTouchscreenDevices(screens);
  std::vector<ui::InputDevice> keyboard_devices;
  keyboard_devices.push_back(ui::InputDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, "keyboard"));
  ui::InputDeviceClientTestApi().SetKeyboardDevices(keyboard_devices);
  ASSERT_FALSE(keyboard::IsKeyboardEnabled());
  // Remove the internal keyboard. Virtual keyboard should now show.
  ui::InputDeviceClientTestApi().SetKeyboardDevices({});
  EXPECT_TRUE(keyboard::IsKeyboardEnabled());
  // Replug in the internal keyboard. Virtual keyboard should hide.
  ui::InputDeviceClientTestApi().SetKeyboardDevices(keyboard_devices);
  EXPECT_FALSE(keyboard::IsKeyboardEnabled());
}

TEST_F(VirtualKeyboardControllerAutoTest, DisabledIfNoTouchScreen) {
  std::vector<ui::TouchscreenDevice> devices;
  // Add a touchscreen. Keyboard should deploy.
  devices.push_back(
      ui::TouchscreenDevice(1, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL,
                            "Touchscreen", gfx::Size(800, 600), 0));
  ui::InputDeviceClientTestApi().SetTouchscreenDevices(devices);
  EXPECT_TRUE(keyboard::IsKeyboardEnabled());
  // Remove touchscreen. Keyboard should hide.
  ui::InputDeviceClientTestApi().SetTouchscreenDevices({});
  EXPECT_FALSE(keyboard::IsKeyboardEnabled());
}

TEST_F(VirtualKeyboardControllerAutoTest, SuppressedIfExternalKeyboardPresent) {
  std::vector<ui::TouchscreenDevice> screens;
  screens.push_back(ui::TouchscreenDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, "Touchscreen",
      gfx::Size(1024, 768), 0, false /* has_stylus */));
  ui::InputDeviceClientTestApi().SetTouchscreenDevices(screens);
  std::vector<ui::InputDevice> keyboard_devices;
  keyboard_devices.push_back(ui::InputDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, "keyboard"));
  ui::InputDeviceClientTestApi().SetKeyboardDevices(keyboard_devices);
  ASSERT_FALSE(keyboard::IsKeyboardEnabled());
  ASSERT_TRUE(notified());
  ASSERT_TRUE(IsVirtualKeyboardSuppressed());
  // Toggle show keyboard. Keyboard should be visible.
  ResetObserver();
  Shell::Get()->virtual_keyboard_controller()->ToggleIgnoreExternalKeyboard();
  ASSERT_TRUE(keyboard::IsKeyboardEnabled());
  ASSERT_TRUE(notified());
  ASSERT_TRUE(IsVirtualKeyboardSuppressed());
  // Toggle show keyboard. Keyboard should be hidden.
  ResetObserver();
  Shell::Get()->virtual_keyboard_controller()->ToggleIgnoreExternalKeyboard();
  ASSERT_FALSE(keyboard::IsKeyboardEnabled());
  ASSERT_TRUE(notified());
  ASSERT_TRUE(IsVirtualKeyboardSuppressed());
  // Remove external keyboard. Should be notified that the keyboard is not
  // suppressed.
  ResetObserver();
  ui::InputDeviceClientTestApi().SetKeyboardDevices({});
  ASSERT_TRUE(keyboard::IsKeyboardEnabled());
  ASSERT_TRUE(notified());
  ASSERT_FALSE(IsVirtualKeyboardSuppressed());
}

// Tests handling multiple keyboards. Catches crbug.com/430252
TEST_F(VirtualKeyboardControllerAutoTest, HandleMultipleKeyboardsPresent) {
  std::vector<ui::InputDevice> keyboards;
  keyboards.push_back(ui::InputDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, "keyboard"));
  keyboards.push_back(ui::InputDevice(
      2, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, "keyboard"));
  keyboards.push_back(ui::InputDevice(
      3, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, "keyboard"));
  ui::InputDeviceClientTestApi().SetKeyboardDevices(keyboards);
  ASSERT_FALSE(keyboard::IsKeyboardEnabled());
}

// Tests tablet mode interaction without disabling the internal keyboard.
TEST_F(VirtualKeyboardControllerAutoTest, EnabledDuringTabletMode) {
  std::vector<ui::TouchscreenDevice> screens;
  screens.push_back(
      ui::TouchscreenDevice(1, ui::InputDeviceType::INPUT_DEVICE_INTERNAL,
                            "Touchscreen", gfx::Size(1024, 768), 0));
  ui::InputDeviceClientTestApi().SetTouchscreenDevices(screens);
  std::vector<ui::InputDevice> keyboard_devices;
  keyboard_devices.push_back(ui::InputDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, "Keyboard"));
  ui::InputDeviceClientTestApi().SetKeyboardDevices(keyboard_devices);
  ASSERT_FALSE(keyboard::IsKeyboardEnabled());
  // Toggle tablet mode on.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  ASSERT_TRUE(keyboard::IsKeyboardEnabled());
  // Toggle tablet mode off.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  ASSERT_FALSE(keyboard::IsKeyboardEnabled());
}

// Tests that keyboard gets suppressed in tablet mode.
TEST_F(VirtualKeyboardControllerAutoTest, SuppressedInMaximizedMode) {
  std::vector<ui::TouchscreenDevice> screens;
  screens.push_back(
      ui::TouchscreenDevice(1, ui::InputDeviceType::INPUT_DEVICE_INTERNAL,
                            "Touchscreen", gfx::Size(1024, 768), 0));
  ui::InputDeviceClientTestApi().SetTouchscreenDevices(screens);
  std::vector<ui::InputDevice> keyboard_devices;
  keyboard_devices.push_back(ui::InputDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_INTERNAL, "Keyboard"));
  keyboard_devices.push_back(ui::InputDevice(
      2, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, "Keyboard"));
  ui::InputDeviceClientTestApi().SetKeyboardDevices(keyboard_devices);
  // Toggle tablet mode on.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  ASSERT_FALSE(keyboard::IsKeyboardEnabled());
  ASSERT_TRUE(notified());
  ASSERT_TRUE(IsVirtualKeyboardSuppressed());
  // Toggle show keyboard. Keyboard should be visible.
  ResetObserver();
  Shell::Get()->virtual_keyboard_controller()->ToggleIgnoreExternalKeyboard();
  ASSERT_TRUE(keyboard::IsKeyboardEnabled());
  ASSERT_TRUE(notified());
  ASSERT_TRUE(IsVirtualKeyboardSuppressed());
  // Toggle show keyboard. Keyboard should be hidden.
  ResetObserver();
  Shell::Get()->virtual_keyboard_controller()->ToggleIgnoreExternalKeyboard();
  ASSERT_FALSE(keyboard::IsKeyboardEnabled());
  ASSERT_TRUE(notified());
  ASSERT_TRUE(IsVirtualKeyboardSuppressed());
  // Remove external keyboard. Should be notified that the keyboard is not
  // suppressed.
  ResetObserver();
  keyboard_devices.pop_back();
  ui::InputDeviceClientTestApi().SetKeyboardDevices(keyboard_devices);
  ASSERT_TRUE(keyboard::IsKeyboardEnabled());
  ASSERT_TRUE(notified());
  ASSERT_FALSE(IsVirtualKeyboardSuppressed());
  // Toggle tablet mode oFF.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  ASSERT_FALSE(keyboard::IsKeyboardEnabled());
}

class VirtualKeyboardControllerAlwaysEnabledTest
    : public VirtualKeyboardControllerAutoTest {
 public:
  VirtualKeyboardControllerAlwaysEnabledTest()
      : VirtualKeyboardControllerAutoTest() {}
  ~VirtualKeyboardControllerAlwaysEnabledTest() override = default;

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        keyboard::switches::kEnableVirtualKeyboard);
    VirtualKeyboardControllerAutoTest::SetUp();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(VirtualKeyboardControllerAlwaysEnabledTest);
};

// Tests that the controller cannot suppress the keyboard if the virtual
// keyboard always enabled flag is active.
TEST_F(VirtualKeyboardControllerAlwaysEnabledTest, DoesNotSuppressKeyboard) {
  std::vector<ui::TouchscreenDevice> screens;
  screens.push_back(
      ui::TouchscreenDevice(1, ui::InputDeviceType::INPUT_DEVICE_INTERNAL,
                            "Touchscreen", gfx::Size(1024, 768), 0));
  ui::InputDeviceClientTestApi().SetTouchscreenDevices(screens);
  std::vector<ui::InputDevice> keyboard_devices;
  keyboard_devices.push_back(ui::InputDevice(
      1, ui::InputDeviceType::INPUT_DEVICE_EXTERNAL, "keyboard"));
  ui::InputDeviceClientTestApi().SetKeyboardDevices(keyboard_devices);
  ASSERT_TRUE(keyboard::IsKeyboardEnabled());
}

}  // namespace ash
