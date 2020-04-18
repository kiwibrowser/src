// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCELERATORS_SPOKEN_FEEDBACK_TOGGLER_H_
#define ASH_ACCELERATORS_SPOKEN_FEEDBACK_TOGGLER_H_

#include <memory>

#include "ash/accelerators/key_hold_detector.h"
#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/events/event_handler.h"

namespace ui {
class KeyEvent;
}

namespace ash {

// A KeyHoldDetector delegate to toggle spoken feedback.
class ASH_EXPORT SpokenFeedbackToggler : public KeyHoldDetector::Delegate {
 public:
  static bool IsEnabled();
  static void SetEnabled(bool enabled);
  static std::unique_ptr<ui::EventHandler> CreateHandler();

  // A scoped object to enable and disable the magnifier accelerator for test.
  class ScopedEnablerForTest {
   public:
    ScopedEnablerForTest() { SetEnabled(true); }
    ~ScopedEnablerForTest() { SetEnabled(false); }

   private:
    DISALLOW_COPY_AND_ASSIGN(ScopedEnablerForTest);
  };

 private:
  // KeyHoldDetector overrides:
  bool ShouldProcessEvent(const ui::KeyEvent* event) const override;
  bool IsStartEvent(const ui::KeyEvent* event) const override;
  bool ShouldStopEventPropagation() const override;
  void OnKeyHold(const ui::KeyEvent* event) override;
  void OnKeyUnhold(const ui::KeyEvent* event) override;

  SpokenFeedbackToggler();
  ~SpokenFeedbackToggler() override;

  bool toggled_;

  DISALLOW_COPY_AND_ASSIGN(SpokenFeedbackToggler);
};

}  // namespace ash

#endif  // ASH_ACCELERATORS_SPOKEN_FEEDBACK_TOGGLER_H_
