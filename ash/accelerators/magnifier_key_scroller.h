// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCELERATORS_MAGNIFIER_KEY_SCROLLER_H_
#define ASH_ACCELERATORS_MAGNIFIER_KEY_SCROLLER_H_

#include <memory>

#include "ash/accelerators/key_hold_detector.h"
#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/events/event_handler.h"

namespace ui {
class KeyEvent;
}

namespace ash {

// A KeyHoldDetector delegate to control control magnified screen.
class ASH_EXPORT MagnifierKeyScroller : public KeyHoldDetector::Delegate {
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

  MagnifierKeyScroller();
  ~MagnifierKeyScroller() override;

  DISALLOW_COPY_AND_ASSIGN(MagnifierKeyScroller);
};

}  // namespace ash

#endif  // ASH_ACCELERATORS_MAGNIFIER_KEY_SCROLLER_H_
