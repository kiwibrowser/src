// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/aura/overscroll_window_delegate.h"

#include "base/macros.h"
#include "content/browser/renderer_host/overscroll_controller_delegate.h"
#include "content/public/browser/overscroll_configuration.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/window.h"
#include "ui/events/gesture_detection/gesture_configuration.h"
#include "ui/events/test/event_generator.h"

namespace content {

namespace {
const int kTestDisplayWidth = 800;
const int kTestWindowWidth = 600;
}

class OverscrollWindowDelegateTest : public aura::test::AuraTestBase,
                                     public OverscrollControllerDelegate {
 public:
  OverscrollWindowDelegateTest()
      : window_(nullptr),
        overscroll_complete_(false),
        overscroll_started_(false),
        mode_changed_(false),
        current_mode_(OVERSCROLL_NONE),
        touch_start_threshold_(OverscrollConfig::GetThreshold(
            OverscrollConfig::Threshold::kStartTouchscreen)),
        touch_complete_threshold_(OverscrollConfig::GetThreshold(
            OverscrollConfig::Threshold::kCompleteTouchscreen)) {}

  ~OverscrollWindowDelegateTest() override {}

  void Reset() {
    overscroll_complete_ = false;
    overscroll_started_ = false;
    mode_changed_ = false;
    current_mode_ = OVERSCROLL_NONE;
    window_.reset(CreateNormalWindow(
        0, root_window(), new OverscrollWindowDelegate(this, gfx::Image())));
    window_->SetBounds(gfx::Rect(0, 0, kTestWindowWidth, kTestWindowWidth));
  }

  // Accessors.
  aura::Window* window() { return window_.get(); }

  bool overscroll_complete() { return overscroll_complete_; }
  bool overscroll_started() { return overscroll_started_; }
  bool mode_changed() { return mode_changed_; }

  OverscrollMode current_mode() { return current_mode_; }

  float touch_start_threshold() { return touch_start_threshold_; }

  float touch_complete_threshold() {
    return kTestDisplayWidth * touch_complete_threshold_;
  }

 protected:
  // aura::test::AuraTestBase:
  void SetUp() override {
    aura::test::AuraTestBase::SetUp();
    Reset();
    ui::GestureConfiguration::GetInstance()
        ->set_max_touch_move_in_pixels_for_click(0);
  }

  void TearDown() override {
    window_.reset();
    aura::test::AuraTestBase::TearDown();
  }

 private:
  // OverscrollControllerDelegate:
  gfx::Size GetDisplaySize() const override {
    return gfx::Size(kTestDisplayWidth, kTestDisplayWidth);
  }

  bool OnOverscrollUpdate(float delta_x, float delta_y) override {
    return true;
  }

  void OnOverscrollComplete(OverscrollMode overscroll_mode) override {
    overscroll_complete_ = true;
  }

  void OnOverscrollModeChange(OverscrollMode old_mode,
                              OverscrollMode new_mode,
                              OverscrollSource source,
                              cc::OverscrollBehavior behavior) override {
    mode_changed_ = true;
    current_mode_ = new_mode;
    if (current_mode_ != OVERSCROLL_NONE)
      overscroll_started_ = true;
  }

  base::Optional<float> GetMaxOverscrollDelta() const override {
    return base::nullopt;
  }

  // Window in which the overscroll window delegate is installed.
  std::unique_ptr<aura::Window> window_;

  // State flags.
  bool overscroll_complete_;
  bool overscroll_started_;
  bool mode_changed_;
  OverscrollMode current_mode_;

  // Config defined constants.
  const float touch_start_threshold_;
  const float touch_complete_threshold_;

  DISALLOW_COPY_AND_ASSIGN(OverscrollWindowDelegateTest);
};

// Tests that the basic overscroll gesture works and sends updates to the
// delegate.
TEST_F(OverscrollWindowDelegateTest, BasicOverscroll) {
  ui::test::EventGenerator generator(root_window());

  // Start an OVERSCROLL_EAST gesture.
  generator.GestureScrollSequence(
      gfx::Point(0, 0), gfx::Point(touch_complete_threshold() + 1, 10),
      base::TimeDelta::FromMilliseconds(10), 10);
  EXPECT_TRUE(overscroll_started());
  EXPECT_EQ(current_mode(), OVERSCROLL_EAST);
  EXPECT_TRUE(overscroll_complete());

  Reset();
  // Start an OVERSCROLL_WEST gesture.
  generator.GestureScrollSequence(gfx::Point(touch_complete_threshold() + 1, 0),
                                  gfx::Point(0, 0),
                                  base::TimeDelta::FromMilliseconds(10), 10);
  EXPECT_TRUE(overscroll_started());
  EXPECT_EQ(current_mode(), OVERSCROLL_WEST);
  EXPECT_TRUE(overscroll_complete());
}

// Verifies that the OverscrollWindowDelegate direction is set correctly during
// an overscroll.
TEST_F(OverscrollWindowDelegateTest, BasicOverscrollModes) {
  ui::test::EventGenerator generator(root_window());
  OverscrollWindowDelegate* delegate =
      static_cast<OverscrollWindowDelegate*>(window()->delegate());

  // Start pressing a touch, but do not start the gesture yet.
  generator.MoveTouch(gfx::Point(0, 0));
  generator.PressTouch();
  EXPECT_EQ(delegate->overscroll_mode_, OVERSCROLL_NONE);

  // Slide the touch to the right.
  generator.MoveTouch(gfx::Point(touch_complete_threshold() + 1, 0));
  EXPECT_EQ(delegate->overscroll_mode_, OVERSCROLL_EAST);

  // Complete the gesture.
  generator.ReleaseTouch();
  EXPECT_EQ(delegate->overscroll_mode_, OVERSCROLL_NONE);
  EXPECT_TRUE(overscroll_complete());

  // Start another overscroll.
  generator.MoveTouch(gfx::Point(touch_complete_threshold() + 1, 0));
  generator.PressTouch();
  EXPECT_EQ(delegate->overscroll_mode_, OVERSCROLL_NONE);

  // Slide the touch to the left.
  generator.MoveTouch(gfx::Point(0, 0));
  EXPECT_EQ(delegate->overscroll_mode_, OVERSCROLL_WEST);

  // Complete the gesture.
  generator.ReleaseTouch();
  EXPECT_EQ(delegate->overscroll_mode_, OVERSCROLL_NONE);
  EXPECT_TRUE(overscroll_complete());

  // Generate a mouse events which normally cancel the overscroll. Confirm
  // that superfluous mode changed events are not dispatched.
  Reset();
  generator.PressLeftButton();
  generator.MoveMouseTo(gfx::Point(10, 10));
  EXPECT_FALSE(mode_changed());
}

// Tests that the overscroll does not start until the gesture gets past a
// particular threshold.
TEST_F(OverscrollWindowDelegateTest, OverscrollThreshold) {
  ui::test::EventGenerator generator(root_window());

  // Start an OVERSCROLL_EAST gesture.
  generator.GestureScrollSequence(gfx::Point(0, 0),
                                  gfx::Point(touch_start_threshold(), 0),
                                  base::TimeDelta::FromMilliseconds(10), 10);
  EXPECT_FALSE(overscroll_started());
  EXPECT_EQ(current_mode(), OVERSCROLL_NONE);
  EXPECT_FALSE(overscroll_complete());

  Reset();
  // Start an OVERSCROLL_WEST gesture.
  generator.GestureScrollSequence(gfx::Point(touch_start_threshold(), 0),
                                  gfx::Point(0, 0),
                                  base::TimeDelta::FromMilliseconds(10), 10);
  EXPECT_FALSE(overscroll_started());
  EXPECT_EQ(current_mode(), OVERSCROLL_NONE);
  EXPECT_FALSE(overscroll_complete());
}

// Tests that the overscroll is aborted if the gesture does not get past the
// completion threshold.
TEST_F(OverscrollWindowDelegateTest, AbortOverscrollThreshold) {
  ui::test::EventGenerator generator(root_window());

  // Start an OVERSCROLL_EAST gesture.
  generator.GestureScrollSequence(gfx::Point(0, 0),
                                  gfx::Point(touch_start_threshold() + 1, 0),
                                  base::TimeDelta::FromMilliseconds(10), 10);
  EXPECT_TRUE(overscroll_started());
  EXPECT_EQ(current_mode(), OVERSCROLL_NONE);
  EXPECT_FALSE(overscroll_complete());

  Reset();
  // Start an OVERSCROLL_WEST gesture.
  generator.GestureScrollSequence(gfx::Point(touch_start_threshold() + 1, 0),
                                  gfx::Point(0, 0),
                                  base::TimeDelta::FromMilliseconds(10), 10);
  EXPECT_TRUE(overscroll_started());
  EXPECT_EQ(current_mode(), OVERSCROLL_NONE);
  EXPECT_FALSE(overscroll_complete());
}

// Tests that the overscroll is aborted if the delegate receives some other
// event.
TEST_F(OverscrollWindowDelegateTest, EventAbortsOverscroll) {
  ui::test::EventGenerator generator(root_window());
  // Start an OVERSCROLL_EAST gesture, without releasing touch.
  generator.set_current_location(gfx::Point(0, 0));
  generator.PressTouch();
  int touch_x = touch_start_threshold() + 1;
  generator.MoveTouch(gfx::Point(touch_x, 0));
  EXPECT_TRUE(overscroll_started());
  EXPECT_EQ(current_mode(), OVERSCROLL_EAST);
  EXPECT_FALSE(overscroll_complete());

  // Dispatch a mouse event, the overscroll should be cancelled.
  generator.PressLeftButton();
  EXPECT_EQ(current_mode(), OVERSCROLL_NONE);
  EXPECT_FALSE(overscroll_complete());

  // We should be able to restart the overscroll without lifting the finger.
  generator.MoveTouch(gfx::Point(touch_x + touch_start_threshold() + 1, 0));
  EXPECT_EQ(current_mode(), OVERSCROLL_EAST);
  EXPECT_FALSE(overscroll_complete());
}

}  // namespace content
