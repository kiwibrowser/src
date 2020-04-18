// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_POWER_BUTTON_SCREENSHOT_CONTROLLER_H_
#define ASH_SYSTEM_POWER_POWER_BUTTON_SCREENSHOT_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "ui/events/event_handler.h"

namespace base {
class TickClock;
}  // namespace base

namespace ash {

// Handles power button screenshot accelerator. The screenshot condition is
// pressing power button and volume down key simultaneously, similar to Android
// phones.
class ASH_EXPORT PowerButtonScreenshotController : public ui::EventHandler {
 public:
  // Time that volume down key and power button must be pressed within this
  // interval of each other to make a screenshot.
  static constexpr base::TimeDelta kScreenshotChordDelay =
      base::TimeDelta::FromMilliseconds(150);

  explicit PowerButtonScreenshotController(const base::TickClock* tick_clock);
  ~PowerButtonScreenshotController() override;

  // Returns true if power button event is consumed by |this|, otherwise false.
  bool OnPowerButtonEvent(bool down, const base::TimeTicks& timestamp);

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;

 private:
  friend class PowerButtonScreenshotControllerTestApi;

  // Helper method used to intercept power button event or volume down key event
  // to check screenshot chord condition. Return true if screenshot is taken to
  // indicate that power button and volume down key is consumed by screenshot.
  bool InterceptScreenshotChord();

  // Called by |volume_down_timer_| to perform volume down accelerator.
  void OnVolumeDownTimeout();

  // True if volume down key is pressed.
  bool volume_down_key_pressed_ = false;

  // True if volume up key is pressed.
  bool volume_up_key_pressed_ = false;

  // True if volume down key is consumed by screenshot accelerator.
  bool consume_volume_down_ = false;

  // True if power button is pressed.
  bool power_button_pressed_ = false;

  // Saves the most recent volume down key pressed time.
  base::TimeTicks volume_down_key_pressed_time_;

  // Saves the most recent power button pressed time.
  base::TimeTicks power_button_pressed_time_;

  // Started when volume down key is pressed and power button is not pressed.
  // Stopped when power button is pressed. Runs OnVolumeDownTimeout to perform a
  // volume down accelerator.
  base::OneShotTimer volume_down_timer_;

  // Time source for performed action times.
  const base::TickClock* tick_clock_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(PowerButtonScreenshotController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_POWER_BUTTON_SCREENSHOT_CONTROLLER_H_
