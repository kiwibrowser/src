// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/aura/overscroll_window_animation.h"

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/window.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/compositor/test/layer_animator_test_controller.h"

namespace content {

class OverscrollWindowAnimationTest
    : public OverscrollWindowAnimation::Delegate,
      public aura::test::AuraTestBase {
 public:
  OverscrollWindowAnimationTest()
      : create_window_(true),
        overscroll_started_(false),
        overscroll_completing_(false),
        overscroll_completed_(false),
        overscroll_aborted_(false),
        main_window_(nullptr) {}

  ~OverscrollWindowAnimationTest() override {}

  OverscrollWindowAnimation* owa() { return owa_.get(); }

  // Set to true to return a window in the Create*Window functions, false to
  // return null.
  void set_create_window(bool create_window) { create_window_ = create_window; }

  // The following functions indicate if the events have been called on this
  // delegate.
  bool overscroll_started() { return overscroll_started_; }
  bool overscroll_completing() { return overscroll_completing_; }
  bool overscroll_completed() { return overscroll_completed_; }
  bool overscroll_aborted() { return overscroll_aborted_; }

  void ResetFlags() {
    overscroll_started_ = false;
    overscroll_completing_ = false;
    overscroll_completed_ = false;
    overscroll_aborted_ = false;
  }

 protected:
  // aura::test::AuraTestBase:
  void SetUp() override {
    aura::test::AuraTestBase::SetUp();
    main_window_.reset(CreateNormalWindow(0, root_window(), nullptr));
    ResetFlags();
    create_window_ = true;
    last_window_id_ = 0;
    owa_.reset(new OverscrollWindowAnimation(this));
  }

  void TearDown() override {
    owa_.reset();
    main_window_.reset();
    aura::test::AuraTestBase::TearDown();
  }

  // OverscrollWindowAnimation::Delegate:
  std::unique_ptr<aura::Window> CreateFrontWindow(
      const gfx::Rect& bounds) override {
    return CreateSlideWindow(bounds);
  }

  std::unique_ptr<aura::Window> CreateBackWindow(
      const gfx::Rect& bounds) override {
    return CreateSlideWindow(bounds);
  }

  aura::Window* GetMainWindow() const override { return main_window_.get(); }

  void OnOverscrollCompleting() override { overscroll_completing_ = true; }

  void OnOverscrollCompleted(std::unique_ptr<aura::Window> window) override {
    overscroll_completed_ = true;
  }

  void OnOverscrollCancelled() override { overscroll_aborted_ = true; }

 private:
  // The overscroll window animation under test.
  std::unique_ptr<OverscrollWindowAnimation> owa_;

  std::unique_ptr<aura::Window> CreateSlideWindow(const gfx::Rect& bounds) {
    overscroll_started_ = true;
    if (create_window_) {
      std::unique_ptr<aura::Window> window(
          CreateNormalWindow(++last_window_id_, root_window(), nullptr));
      window->SetBounds(bounds);
      return window;
    }
    return nullptr;
  }

  // Controls if we return a window for the window creation callbacks or not.
  bool create_window_;

  // State flags.
  bool overscroll_started_;
  bool overscroll_completing_;
  bool overscroll_completed_;
  bool overscroll_aborted_;

  int last_window_id_;

  // The dummy target window we provide.
  std::unique_ptr<aura::Window> main_window_;

  DISALLOW_COPY_AND_ASSIGN(OverscrollWindowAnimationTest);
};

// Tests a simple overscroll gesture.
TEST_F(OverscrollWindowAnimationTest, BasicOverscroll) {
  EXPECT_FALSE(owa()->is_active());
  EXPECT_FALSE(overscroll_started());
  EXPECT_FALSE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());

  // Start an OVERSCROLL_EAST gesture.
  owa()->OnOverscrollModeChange(OVERSCROLL_NONE, OVERSCROLL_EAST,
                                OverscrollSource::TOUCHPAD,
                                cc::OverscrollBehavior());
  EXPECT_TRUE(owa()->is_active());
  EXPECT_EQ(OverscrollSource::TOUCHPAD, owa()->overscroll_source());
  EXPECT_TRUE(overscroll_started());
  EXPECT_FALSE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());

  // Complete the overscroll.
  owa()->OnOverscrollComplete(OVERSCROLL_EAST);
  EXPECT_FALSE(owa()->is_active());
  EXPECT_TRUE(overscroll_started());
  EXPECT_TRUE(overscroll_completing());
  EXPECT_TRUE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());
}

// Tests aborting an overscroll gesture.
TEST_F(OverscrollWindowAnimationTest, BasicAbort) {
  // Start an OVERSCROLL_EAST gesture.
  owa()->OnOverscrollModeChange(OVERSCROLL_NONE, OVERSCROLL_EAST,
                                OverscrollSource::TOUCHSCREEN,
                                cc::OverscrollBehavior());
  EXPECT_EQ(OverscrollSource::TOUCHSCREEN, owa()->overscroll_source());
  // Abort the overscroll.
  owa()->OnOverscrollModeChange(OVERSCROLL_EAST, OVERSCROLL_NONE,
                                OverscrollSource::TOUCHSCREEN,
                                cc::OverscrollBehavior());
  EXPECT_EQ(OverscrollSource::NONE, owa()->overscroll_source());
  EXPECT_FALSE(owa()->is_active());
  EXPECT_TRUE(overscroll_started());
  EXPECT_FALSE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_TRUE(overscroll_aborted());
}

// Tests starting an overscroll gesture when the slide window cannot be created.
TEST_F(OverscrollWindowAnimationTest, BasicCannotNavigate) {
  set_create_window(false);
  // Start an OVERSCROLL_EAST gesture.
  owa()->OnOverscrollModeChange(OVERSCROLL_NONE, OVERSCROLL_EAST,
                                OverscrollSource::TOUCHPAD,
                                cc::OverscrollBehavior());
  EXPECT_FALSE(owa()->is_active());
  EXPECT_EQ(OverscrollSource::NONE, owa()->overscroll_source());
  EXPECT_TRUE(overscroll_started());
  EXPECT_FALSE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());
}

// Tests starting an overscroll gesture while another one was in progress
// completes the first one.
TEST_F(OverscrollWindowAnimationTest, NewOverscrollCompletesPreviousGesture) {
  // This test requires a normal animation duration so that
  // OnImplicitAnimationsCancelled is not called as soon as the first overscroll
  // finishes.
  ui::ScopedAnimationDurationScaleMode normal_duration(
      ui::ScopedAnimationDurationScaleMode::NORMAL_DURATION);
  ui::LayerAnimator* animator = GetMainWindow()->layer()->GetAnimator();
  ui::ScopedLayerAnimationSettings settings(animator);
  animator->set_disable_timer_for_test(true);
  ui::LayerAnimatorTestController test_controller(animator);
  // Start an OVERSCROLL_EAST gesture.
  owa()->OnOverscrollModeChange(OVERSCROLL_NONE, OVERSCROLL_EAST,
                                OverscrollSource::TOUCHPAD,
                                cc::OverscrollBehavior());

  // Finishes the OVERSCROLL_EAST gesture. At this point the window should be
  // being animated to its final position.
  owa()->OnOverscrollComplete(OVERSCROLL_EAST);
  EXPECT_TRUE(owa()->is_active());
  EXPECT_EQ(OverscrollSource::TOUCHPAD, owa()->overscroll_source());
  EXPECT_TRUE(overscroll_started());
  EXPECT_TRUE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());

  // Start another OVERSCROLL_EAST gesture.
  owa()->OnOverscrollModeChange(OVERSCROLL_NONE, OVERSCROLL_EAST,
                                OverscrollSource::TOUCHSCREEN,
                                cc::OverscrollBehavior());
  EXPECT_TRUE(owa()->is_active());
  EXPECT_EQ(OverscrollSource::TOUCHSCREEN, owa()->overscroll_source());
  EXPECT_TRUE(overscroll_started());
  EXPECT_TRUE(overscroll_completing());
  EXPECT_TRUE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());

  // Complete the overscroll gesture.
  ResetFlags();
  owa()->OnOverscrollComplete(OVERSCROLL_EAST);

  base::TimeDelta duration = settings.GetTransitionDuration();
  test_controller.StartThreadedAnimationsIfNeeded();
  base::TimeTicks start_time = base::TimeTicks::Now();

  // Halfway through the animation, OverscrollCompleting should have been fired.
  animator->Step(start_time + duration / 2);
  EXPECT_TRUE(owa()->is_active());
  EXPECT_EQ(OverscrollSource::TOUCHSCREEN, owa()->overscroll_source());
  EXPECT_FALSE(overscroll_started());
  EXPECT_TRUE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());

  // The animation has finished, OverscrollCompleted should have been fired.
  animator->Step(start_time + duration);
  EXPECT_FALSE(owa()->is_active());
  EXPECT_EQ(OverscrollSource::NONE, owa()->overscroll_source());
  EXPECT_FALSE(overscroll_started());
  EXPECT_TRUE(overscroll_completing());
  EXPECT_TRUE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());
}

TEST_F(OverscrollWindowAnimationTest, OverscrollBehaviorAutoAllowsOverscroll) {
  EXPECT_FALSE(owa()->is_active());
  EXPECT_FALSE(overscroll_started());
  EXPECT_FALSE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());

  cc::OverscrollBehavior overscroll_behavior;
  overscroll_behavior.x = cc::OverscrollBehavior::OverscrollBehaviorType::
      kOverscrollBehaviorTypeAuto;
  owa()->OnOverscrollModeChange(OVERSCROLL_NONE, OVERSCROLL_EAST,
                                OverscrollSource::TOUCHPAD,
                                overscroll_behavior);
  EXPECT_TRUE(owa()->is_active());
  EXPECT_EQ(OverscrollSource::TOUCHPAD, owa()->overscroll_source());
  EXPECT_TRUE(overscroll_started());
  EXPECT_FALSE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());
}

TEST_F(OverscrollWindowAnimationTest,
       OverscrollBehaviorContainPreventsOverscroll) {
  EXPECT_FALSE(owa()->is_active());
  EXPECT_FALSE(overscroll_started());
  EXPECT_FALSE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());

  cc::OverscrollBehavior overscroll_behavior;
  overscroll_behavior.x = cc::OverscrollBehavior::OverscrollBehaviorType::
      kOverscrollBehaviorTypeContain;
  owa()->OnOverscrollModeChange(OVERSCROLL_NONE, OVERSCROLL_EAST,
                                OverscrollSource::TOUCHPAD,
                                overscroll_behavior);
  EXPECT_FALSE(owa()->is_active());
  EXPECT_FALSE(overscroll_started());
  EXPECT_FALSE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());
}

TEST_F(OverscrollWindowAnimationTest,
       OverscrollBehaviorNonePreventsOverscroll) {
  EXPECT_FALSE(owa()->is_active());
  EXPECT_FALSE(overscroll_started());
  EXPECT_FALSE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());

  cc::OverscrollBehavior overscroll_behavior;
  overscroll_behavior.x = cc::OverscrollBehavior::OverscrollBehaviorType::
      kOverscrollBehaviorTypeNone;
  owa()->OnOverscrollModeChange(OVERSCROLL_NONE, OVERSCROLL_EAST,
                                OverscrollSource::TOUCHPAD,
                                overscroll_behavior);
  EXPECT_FALSE(owa()->is_active());
  EXPECT_FALSE(overscroll_started());
  EXPECT_FALSE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());
}

TEST_F(OverscrollWindowAnimationTest,
       OverscrollBehaviorNoneOnYAllowsOverscroll) {
  EXPECT_FALSE(owa()->is_active());
  EXPECT_FALSE(overscroll_started());
  EXPECT_FALSE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());

  cc::OverscrollBehavior overscroll_behavior;
  overscroll_behavior.x = cc::OverscrollBehavior::OverscrollBehaviorType::
      kOverscrollBehaviorTypeAuto;
  overscroll_behavior.y = cc::OverscrollBehavior::OverscrollBehaviorType::
      kOverscrollBehaviorTypeNone;
  owa()->OnOverscrollModeChange(OVERSCROLL_NONE, OVERSCROLL_EAST,
                                OverscrollSource::TOUCHPAD,
                                overscroll_behavior);
  EXPECT_TRUE(owa()->is_active());
  EXPECT_EQ(OverscrollSource::TOUCHPAD, owa()->overscroll_source());
  EXPECT_TRUE(overscroll_started());
  EXPECT_FALSE(overscroll_completing());
  EXPECT_FALSE(overscroll_completed());
  EXPECT_FALSE(overscroll_aborted());
}

}  // namespace content
