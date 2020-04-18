// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_screenshot_controller.h"

#include <memory>

#include "ash/login_status.h"
#include "ash/shell.h"
#include "ash/system/power/power_button_controller.h"
#include "ash/system/power/power_button_controller_test_api.h"
#include "ash/system/power/power_button_screenshot_controller_test_api.h"
#include "ash/system/power/power_button_test_base.h"
#include "ash/test_screenshot_delegate.h"
#include "ash/wm/lock_state_controller_test_api.h"
#include "base/test/simple_test_tick_clock.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "ui/events/event.h"

namespace ash {

// Test fixture used for testing power button screenshot behavior under tablet
// power button.
class PowerButtonScreenshotControllerTest : public PowerButtonTestBase {
 public:
  PowerButtonScreenshotControllerTest() = default;
  ~PowerButtonScreenshotControllerTest() override = default;

  // PowerButtonTestBase:
  void SetUp() override {
    PowerButtonTestBase::SetUp();
    InitPowerButtonControllerMembers(
        chromeos::PowerManagerClient::TabletMode::ON);
    InitScreenshotTestApi();
    screenshot_delegate_ = GetScreenshotDelegate();
    EnableTabletMode(true);

    // Advance a duration longer than |kIgnorePowerButtonAfterResumeDelay| to
    // avoid events being ignored.
    tick_clock_.Advance(
        PowerButtonController::kIgnorePowerButtonAfterResumeDelay +
        base::TimeDelta::FromMilliseconds(2));
  }

 protected:
  // PowerButtonTestBase:
  void PressKey(ui::KeyboardCode key_code) override {
    last_key_event_ = std::make_unique<ui::KeyEvent>(ui::ET_KEY_PRESSED,
                                                     key_code, ui::EF_NONE);
    screenshot_controller_->OnKeyEvent(last_key_event_.get());
  }
  void ReleaseKey(ui::KeyboardCode key_code) override {
    last_key_event_ = std::make_unique<ui::KeyEvent>(ui::ET_KEY_RELEASED,
                                                     key_code, ui::EF_NONE);
    screenshot_controller_->OnKeyEvent(last_key_event_.get());
  }

  void InitScreenshotTestApi() {
    screenshot_test_api_ =
        std::make_unique<PowerButtonScreenshotControllerTestApi>(
            screenshot_controller_);
  }

  int GetScreenshotCount() const {
    return screenshot_delegate_->handle_take_screenshot_count();
  }

  void ResetScreenshotCount() {
    return screenshot_delegate_->reset_handle_take_screenshot_count();
  }

  bool LastKeyConsumed() const {
    DCHECK(last_key_event_);
    return last_key_event_->stopped_propagation();
  }

  TestScreenshotDelegate* screenshot_delegate_ = nullptr;  // Not owned.
  std::unique_ptr<PowerButtonScreenshotControllerTestApi> screenshot_test_api_;

  // Stores the last key event. Can be NULL if not set through PressKey() or
  // ReleaseKey().
  std::unique_ptr<ui::KeyEvent> last_key_event_;

  DISALLOW_COPY_AND_ASSIGN(PowerButtonScreenshotControllerTest);
};

// Tests power button screenshot accelerator works on tablet mode only.
TEST_F(PowerButtonScreenshotControllerTest, TabletMode) {
  // Tests on tablet mode pressing power button and volume down simultaneously
  // takes a screenshot.
  PressKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  ReleasePowerButton();
  EXPECT_EQ(1, GetScreenshotCount());

  // Tests screenshot handling is not active when not on tablet mode.
  ResetScreenshotCount();
  EnableTabletMode(false);
  PressKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  ReleasePowerButton();
  EXPECT_EQ(0, GetScreenshotCount());
}

// Tests if power-button/volume-down is released before volume-down/power-button
// is pressed, it does not take screenshot.
TEST_F(PowerButtonScreenshotControllerTest, ReleaseBeforeAnotherPressed) {
  // Releases volume down before power button is pressed.
  PressKey(ui::VKEY_VOLUME_DOWN);
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  ReleasePowerButton();
  EXPECT_EQ(0, GetScreenshotCount());

  // Releases power button before volume down is pressed.
  PressPowerButton();
  ReleasePowerButton();
  PressKey(ui::VKEY_VOLUME_DOWN);
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
}

// Tests power button is pressed first and meets screenshot chord condition.
TEST_F(PowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_ScreenshotChord) {
  PressPowerButton();
  tick_clock_.Advance(PowerButtonScreenshotController::kScreenshotChordDelay -
                      base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  // Verifies screenshot is taken, volume down is consumed.
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_TRUE(LastKeyConsumed());
  // Keeps pressing volume down key under screenshot chord condition will not
  // take screenshot again, volume down is also consumed.
  tick_clock_.Advance(base::TimeDelta::FromMilliseconds(1));
  ResetScreenshotCount();
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_TRUE(LastKeyConsumed());
  // Keeps pressing volume down key off screenshot chord condition will not
  // take screenshot and still consume volume down event.
  tick_clock_.Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_TRUE(LastKeyConsumed());

  // Releases power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Releases volume down key, and verifies nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
}

// Tests power button is pressed first, and then volume down pressed but doesn't
// meet screenshot chord condition.
TEST_F(PowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_NoScreenshotChord) {
  PressPowerButton();
  tick_clock_.Advance(PowerButtonScreenshotController::kScreenshotChordDelay +
                      base::TimeDelta::FromMilliseconds(1));
  PressKey(ui::VKEY_VOLUME_DOWN);
  // Verifies screenshot is not taken, volume down is not consumed.
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
  // Keeps pressing volume down key should continue triggerring volume down.
  tick_clock_.Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
  // Releases power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Releases volume down key, and verifies nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
}

// Tests volume key pressed cancels the ongoing power button behavior.
TEST_F(PowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_VolumeKeyCancelPowerButton) {
  // Tests volume down key can stop power button's shutdown timer and power
  // button menu timer.
  PressPowerButton();
  EXPECT_TRUE(power_button_test_api_->PowerButtonMenuTimerIsRunning());
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(power_button_test_api_->PowerButtonMenuTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());

  // Tests volume up key can stop power button's shutdown timer and power button
  // menu timer. Also tests that volume up key is not consumed.
  PressPowerButton();
  EXPECT_TRUE(power_button_test_api_->PowerButtonMenuTimerIsRunning());
  PressKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(LastKeyConsumed());
  EXPECT_FALSE(power_button_test_api_->PowerButtonMenuTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  EXPECT_FALSE(LastKeyConsumed());
}

// Tests volume key pressed can not cancel the started pre-shutdown animation.
TEST_F(PowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_VolumeKeyNotCancelPowerButton) {
  PressPowerButton();
  ASSERT_TRUE(power_button_test_api_->TriggerPowerButtonMenuTimeout());
  EXPECT_TRUE(power_button_test_api_->PreShutdownTimerIsRunning());
  EXPECT_TRUE(power_button_test_api_->TriggerPreShutdownTimeout());
  EXPECT_TRUE(lock_state_test_api_->shutdown_timer_is_running());
  PressKey(ui::VKEY_VOLUME_DOWN);
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_TRUE(lock_state_test_api_->shutdown_timer_is_running());
  ReleasePowerButton();
  EXPECT_FALSE(lock_state_test_api_->shutdown_timer_is_running());
}

// Tests volume down key pressed first and meets screenshot chord condition.
TEST_F(PowerButtonScreenshotControllerTest,
       VolumeDownPressedFirst_ScreenshotChord) {
  // Tests when volume down pressed first, it waits for power button pressed
  // screenshot chord.
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_TRUE(LastKeyConsumed());
  // Presses power button under screenshot chord condition, and verifies that
  // screenshot is taken.
  tick_clock_.Advance(PowerButtonScreenshotController::kScreenshotChordDelay -
                      base::TimeDelta::FromMilliseconds(2));
  PressPowerButton();
  EXPECT_EQ(1, GetScreenshotCount());
  // Keeps pressing volume down key under screenshot chord condition will not
  // take screenshot again, volume down is also consumed.
  tick_clock_.Advance(base::TimeDelta::FromMilliseconds(1));
  ResetScreenshotCount();
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_TRUE(LastKeyConsumed());
  // Keeps pressing volume down key off screenshot chord condition will not take
  // screenshot and still consume volume down event.
  tick_clock_.Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_TRUE(LastKeyConsumed());

  // Releases power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Releases volume down key, and verifies nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
}

// Tests volume down key pressed first, and then power button pressed but
// doesn't meet screenshot chord condition.
TEST_F(PowerButtonScreenshotControllerTest,
       VolumeDownPressedFirst_NoScreenshotChord) {
  // Tests when volume down pressed first, it waits for power button pressed
  // screenshot chord.
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_TRUE(LastKeyConsumed());
  // Advances |tick_clock_| to off screenshot chord point. This will also
  // trigger volume down timer timeout, which will perform a volume down
  // operation.
  tick_clock_.Advance(PowerButtonScreenshotController::kScreenshotChordDelay +
                      base::TimeDelta::FromMilliseconds(1));
  EXPECT_TRUE(screenshot_test_api_->TriggerVolumeDownTimer());
  // Presses power button would not take screenshot.
  PressPowerButton();
  EXPECT_EQ(0, GetScreenshotCount());
  // Keeps pressing volume down key should continue triggerring volume down.
  tick_clock_.Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
  // Releases power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Releases volume down key, and verifies nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
}

// Tests volume key pressed first invalidates the power button behavior.
TEST_F(PowerButtonScreenshotControllerTest,
       VolumeKeyPressedFirst_InvalidateConvertiblePowerButton) {
  // Tests volume down key invalidates the power button behavior.
  PressKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  EXPECT_FALSE(power_button_test_api_->PowerButtonMenuTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());

  // Tests volume up key invalidates the power button behavior. Also
  // tests that volume up key is not consumed.
  PressKey(ui::VKEY_VOLUME_UP);
  PressPowerButton();
  EXPECT_FALSE(LastKeyConsumed());
  EXPECT_FALSE(power_button_test_api_->PowerButtonMenuTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  EXPECT_FALSE(LastKeyConsumed());
}

}  // namespace ash
