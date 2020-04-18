// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accessibility/spoken_feedback_enabler.h"

#include "ash/accessibility/accessibility_controller.h"
#include "ash/shell.h"
#include "ui/events/base_event_utils.h"

namespace ash {

namespace {

// Delay between timer callbacks. Each one plays a tick sound.
constexpr base::TimeDelta kTimerDelay = base::TimeDelta::FromMilliseconds(500);

// The number of ticks of the timer before the first sound is generated.
constexpr int kTimerTicksOfFirstSoundFeedback = 6;

// The number of ticks of the timer before toggling spoken feedback.
constexpr int kTimerTicksToToggleSpokenFeedback = 10;

}  // namespace

SpokenFeedbackEnabler::SpokenFeedbackEnabler() {
  start_time_ = ui::EventTimeForNow();
  timer_.Start(FROM_HERE, kTimerDelay, this, &SpokenFeedbackEnabler::OnTimer);
}

SpokenFeedbackEnabler::~SpokenFeedbackEnabler() {}

void SpokenFeedbackEnabler::OnTimer() {
  base::TimeTicks now = ui::EventTimeForNow();
  double tick_count_f = (now - start_time_) / kTimerDelay;
  int tick_count = roundf(tick_count_f);

  AccessibilityController* delegate = Shell::Get()->accessibility_controller();
  CHECK(delegate);
  if (tick_count >= kTimerTicksOfFirstSoundFeedback &&
      tick_count < kTimerTicksToToggleSpokenFeedback) {
    delegate->PlaySpokenFeedbackToggleCountdown(tick_count);
  } else if (tick_count == kTimerTicksToToggleSpokenFeedback) {
    delegate->SetSpokenFeedbackEnabled(!delegate->IsSpokenFeedbackEnabled(),
                                       A11Y_NOTIFICATION_SHOW);
    timer_.Stop();
  }
}

}  // namespace ash
