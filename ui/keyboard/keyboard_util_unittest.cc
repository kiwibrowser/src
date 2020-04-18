// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/keyboard_util.h"

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_test_util.h"
#include "ui/keyboard/keyboard_ui.h"

namespace keyboard {
namespace {

class KeyboardUtilTest : public testing::Test {
 public:
  KeyboardUtilTest() {}
  ~KeyboardUtilTest() override {}

  // Sets all flags controlling whether the keyboard should be shown to
  // their disabled state.
  void DisableAllFlags() {
    keyboard::SetAccessibilityKeyboardEnabled(false);
    keyboard::SetTouchKeyboardEnabled(false);
    keyboard::SetKeyboardShowOverride(
        keyboard::KEYBOARD_SHOW_OVERRIDE_DISABLED);
    keyboard::SetRequestedKeyboardState(keyboard::KEYBOARD_STATE_DISABLED);
  }

  // Sets all flags controlling whether the keyboard should be shown to
  // their enabled state.
  void EnableAllFlags() {
    keyboard::SetAccessibilityKeyboardEnabled(true);
    keyboard::SetTouchKeyboardEnabled(true);
    keyboard::SetKeyboardShowOverride(keyboard::KEYBOARD_SHOW_OVERRIDE_ENABLED);
    keyboard::SetRequestedKeyboardState(keyboard::KEYBOARD_STATE_ENABLED);
  }

  // Sets all flags controlling whether the keyboard should be shown to
  // their neutral state.
  void ResetAllFlags() {
    keyboard::SetAccessibilityKeyboardEnabled(false);
    keyboard::SetTouchKeyboardEnabled(false);
    keyboard::SetKeyboardShowOverride(keyboard::KEYBOARD_SHOW_OVERRIDE_NONE);
    keyboard::SetRequestedKeyboardState(keyboard::KEYBOARD_STATE_AUTO);
  }

  void SetUp() override { ResetAllFlags(); }

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardUtilTest);
};

}  // namespace

// Tests that we respect the accessibility setting.
TEST_F(KeyboardUtilTest, AlwaysShowIfA11yEnabled) {
  // Disabled by default.
  EXPECT_FALSE(keyboard::IsKeyboardEnabled());
  // If enabled by accessibility, should ignore other flag values.
  DisableAllFlags();
  keyboard::SetAccessibilityKeyboardEnabled(true);
  EXPECT_TRUE(keyboard::IsKeyboardEnabled());
}

// Tests that we respect the policy setting.
TEST_F(KeyboardUtilTest, AlwaysShowIfPolicyEnabled) {
  EXPECT_FALSE(keyboard::IsKeyboardEnabled());
  // If policy is enabled, should ignore other flag values.
  DisableAllFlags();
  keyboard::SetKeyboardShowOverride(keyboard::KEYBOARD_SHOW_OVERRIDE_ENABLED);
  EXPECT_TRUE(keyboard::IsKeyboardEnabled());
}

// Tests that we respect the policy setting.
TEST_F(KeyboardUtilTest, HidesIfPolicyDisabled) {
  EXPECT_FALSE(keyboard::IsKeyboardEnabled());
  EnableAllFlags();
  // Set accessibility to neutral since accessibility has higher precedence.
  keyboard::SetAccessibilityKeyboardEnabled(false);
  EXPECT_TRUE(keyboard::IsKeyboardEnabled());
  // Disable policy. Keyboard should be disabled.
  keyboard::SetKeyboardShowOverride(keyboard::KEYBOARD_SHOW_OVERRIDE_DISABLED);
  EXPECT_FALSE(keyboard::IsKeyboardEnabled());
}

// Tests that the keyboard shows when requested state provided higher priority
// flags have not been set.
TEST_F(KeyboardUtilTest, ShowKeyboardWhenRequested) {
  DisableAllFlags();
  // Remove device policy, which has higher precedence than us.
  keyboard::SetKeyboardShowOverride(keyboard::KEYBOARD_SHOW_OVERRIDE_NONE);
  EXPECT_FALSE(keyboard::IsKeyboardEnabled());
  // Requested should have higher precedence than all the remaining flags.
  keyboard::SetRequestedKeyboardState(keyboard::KEYBOARD_STATE_ENABLED);
  EXPECT_TRUE(keyboard::IsKeyboardEnabled());
}

// Tests that the touch keyboard is hidden when requested state is disabled and
// higher priority flags have not been set.
TEST_F(KeyboardUtilTest, HideKeyboardWhenRequested) {
  EnableAllFlags();
  // Remove higher precedence flags.
  keyboard::SetKeyboardShowOverride(keyboard::KEYBOARD_SHOW_OVERRIDE_NONE);
  keyboard::SetAccessibilityKeyboardEnabled(false);
  EXPECT_TRUE(keyboard::IsKeyboardEnabled());
  // Set requested state to disable. Keyboard should disable.
  keyboard::SetRequestedKeyboardState(keyboard::KEYBOARD_STATE_DISABLED);
  EXPECT_FALSE(keyboard::IsKeyboardEnabled());
}

// SetTouchKeyboardEnabled has the lowest priority, but should still work when
// none of the other flags are enabled.
TEST_F(KeyboardUtilTest, HideKeyboardWhenTouchEnabled) {
  ResetAllFlags();
  EXPECT_FALSE(keyboard::IsKeyboardEnabled());
  keyboard::SetTouchKeyboardEnabled(true);
  EXPECT_TRUE(keyboard::IsKeyboardEnabled());
}

TEST_F(KeyboardUtilTest, UpdateKeyboardConfig) {
  ResetAllFlags();
  keyboard::KeyboardConfig config = keyboard::GetKeyboardConfig();
  EXPECT_TRUE(config.spell_check);
  EXPECT_FALSE(keyboard::UpdateKeyboardConfig(config));

  config.spell_check = false;
  EXPECT_TRUE(keyboard::UpdateKeyboardConfig(config));
  EXPECT_FALSE(keyboard::GetKeyboardConfig().spell_check);

  EXPECT_FALSE(keyboard::UpdateKeyboardConfig(config));
}

TEST_F(KeyboardUtilTest, IsOverscrollEnabled) {
  ResetAllFlags();

  // Return false when keyboard is disabled.
  EXPECT_FALSE(keyboard::IsKeyboardOverscrollEnabled());

  // Enable the virtual keyboard.
  keyboard::SetTouchKeyboardEnabled(true);
  EXPECT_TRUE(keyboard::IsKeyboardOverscrollEnabled());

  // Override overscroll enabled state.
  keyboard::SetKeyboardOverscrollOverride(
      KEYBOARD_OVERSCROLL_OVERRIDE_DISABLED);
  EXPECT_FALSE(keyboard::IsKeyboardOverscrollEnabled());
  keyboard::SetKeyboardOverscrollOverride(KEYBOARD_OVERSCROLL_OVERRIDE_NONE);
  EXPECT_TRUE(keyboard::IsKeyboardOverscrollEnabled());

  // Set keyboard_locked() to true.
  ui::DummyInputMethod input_method;
  KeyboardController::ResetInstance(new KeyboardController(
      std::make_unique<TestKeyboardUI>(&input_method), nullptr));
  KeyboardController* controller = keyboard::KeyboardController::GetInstance();
  controller->set_keyboard_locked(true);
  EXPECT_TRUE(controller->keyboard_locked());
  EXPECT_FALSE(keyboard::IsKeyboardOverscrollEnabled());
  KeyboardController::ResetInstance(nullptr);
}

}  // namespace keyboard
