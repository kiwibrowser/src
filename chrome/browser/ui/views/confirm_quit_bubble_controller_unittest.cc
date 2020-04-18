// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/confirm_quit_bubble_controller.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/timer/mock_timer.h"
#include "chrome/browser/ui/views/confirm_quit_bubble.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/animation/slide_animation.h"

class TestConfirmQuitBubble : public ConfirmQuitBubbleBase {
 public:
  TestConfirmQuitBubble() {}
  ~TestConfirmQuitBubble() override {}

  void Show() override {}
  void Hide() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestConfirmQuitBubble);
};

class TestSlideAnimation : public gfx::SlideAnimation {
 public:
  TestSlideAnimation() : gfx::SlideAnimation(nullptr) {}
  ~TestSlideAnimation() override {}

  void Reset() override {}
  void Reset(double value) override {}
  void Show() override {}
  void Hide() override {}
  void SetSlideDuration(int duration) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestSlideAnimation);
};

class ConfirmQuitBubbleControllerTest : public testing::Test {
 protected:
  void SetUp() override {
    std::unique_ptr<TestConfirmQuitBubble> bubble =
        std::make_unique<TestConfirmQuitBubble>();
    std::unique_ptr<base::MockTimer> timer =
        std::make_unique<base::MockTimer>(false, false);
    bubble_ = bubble.get();
    timer_ = timer.get();
    controller_.reset(new ConfirmQuitBubbleController(
        std::move(bubble), std::move(timer),
        std::make_unique<TestSlideAnimation>()));

    quit_called_ = false;
    controller_->SetQuitActionForTest(base::BindOnce(
        &ConfirmQuitBubbleControllerTest::OnQuit, base::Unretained(this)));
  }

  void TearDown() override { controller_.reset(); }

  void OnQuit() { quit_called_ = true; }

  void SendAccelerator(bool quit, bool press, bool repeat) {
    ui::KeyboardCode key = quit ? ui::VKEY_Q : ui::VKEY_P;
    int modifiers = ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN;
    if (repeat)
      modifiers |= ui::EF_IS_REPEAT;
    ui::Accelerator::KeyState state = press
                                          ? ui::Accelerator::KeyState::PRESSED
                                          : ui::Accelerator::KeyState::RELEASED;
    controller_->HandleKeyboardEvent(ui::Accelerator(key, modifiers, state));
  }

  void PressQuitAccelerator() { SendAccelerator(true, true, false); }

  void ReleaseQuitAccelerator() { SendAccelerator(true, false, false); }

  void RepeatQuitAccelerator() { SendAccelerator(true, true, true); }

  void PressOtherAccelerator() { SendAccelerator(false, true, false); }

  void ReleaseOtherAccelerator() { SendAccelerator(false, false, false); }

  void DeactivateBrowser() { controller_->OnBrowserNoLongerActive(nullptr); }

  std::unique_ptr<ConfirmQuitBubbleController> controller_;

  // Owned by |controller_|.
  TestConfirmQuitBubble* bubble_;

  // Owned by |controller_|.
  base::MockTimer* timer_;

  bool quit_called_ = false;
};

// Pressing and holding the shortcut should quit.
TEST_F(ConfirmQuitBubbleControllerTest, PressAndHold) {
  PressQuitAccelerator();
  EXPECT_TRUE(timer_->IsRunning());
  timer_->Fire();
  EXPECT_FALSE(quit_called_);
  ReleaseQuitAccelerator();
  EXPECT_TRUE(quit_called_);
}

// Pressing the shortcut twice should quit.
TEST_F(ConfirmQuitBubbleControllerTest, DoublePress) {
  PressQuitAccelerator();
  ReleaseQuitAccelerator();
  EXPECT_TRUE(timer_->IsRunning());
  PressQuitAccelerator();
  EXPECT_FALSE(timer_->IsRunning());
  EXPECT_FALSE(quit_called_);
  ReleaseQuitAccelerator();
  EXPECT_TRUE(quit_called_);
}

// Pressing the shortcut once should not quit.
TEST_F(ConfirmQuitBubbleControllerTest, SinglePress) {
  PressQuitAccelerator();
  ReleaseQuitAccelerator();
  EXPECT_TRUE(timer_->IsRunning());
  timer_->Fire();
  EXPECT_FALSE(quit_called_);
}

// Repeated presses should not be counted.
TEST_F(ConfirmQuitBubbleControllerTest, RepeatedPresses) {
  PressQuitAccelerator();
  RepeatQuitAccelerator();
  ReleaseQuitAccelerator();
  EXPECT_TRUE(timer_->IsRunning());
  timer_->Fire();
  EXPECT_FALSE(quit_called_);
}

// Other keys shouldn't matter.
TEST_F(ConfirmQuitBubbleControllerTest, OtherKeyPress) {
  PressQuitAccelerator();
  ReleaseQuitAccelerator();
  PressOtherAccelerator();
  ReleaseOtherAccelerator();
  EXPECT_TRUE(timer_->IsRunning());
  PressQuitAccelerator();
  EXPECT_FALSE(timer_->IsRunning());
  EXPECT_FALSE(quit_called_);
  ReleaseQuitAccelerator();
  EXPECT_TRUE(quit_called_);
}

// The controller state should be reset when the browser loses focus.
TEST_F(ConfirmQuitBubbleControllerTest, BrowserLosesFocus) {
  // Press but don't release the accelerator.
  PressQuitAccelerator();
  EXPECT_TRUE(timer_->IsRunning());
  DeactivateBrowser();
  EXPECT_FALSE(timer_->IsRunning());
  EXPECT_FALSE(quit_called_);
  ReleaseQuitAccelerator();

  // Press and release the accelerator.
  PressQuitAccelerator();
  ReleaseQuitAccelerator();
  EXPECT_TRUE(timer_->IsRunning());
  DeactivateBrowser();
  EXPECT_FALSE(timer_->IsRunning());
  EXPECT_FALSE(quit_called_);

  // Press and hold the accelerator.
  PressQuitAccelerator();
  EXPECT_TRUE(timer_->IsRunning());
  timer_->Fire();
  EXPECT_FALSE(timer_->IsRunning());
  DeactivateBrowser();
  ReleaseQuitAccelerator();
  EXPECT_FALSE(quit_called_);
}
