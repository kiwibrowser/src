// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/confirm_quit_bubble_controller.h"

#include <utility>

#include "base/feature_list.h"
#include "base/memory/singleton.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/confirm_quit_bubble.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/slide_animation.h"

namespace {

constexpr ui::KeyboardCode kAcceleratorKeyCode = ui::VKEY_Q;
constexpr int kAcceleratorModifiers = ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN;

constexpr base::TimeDelta kShowDuration =
    base::TimeDelta::FromMilliseconds(1500);

constexpr base::TimeDelta kWindowFadeOutDuration =
    base::TimeDelta::FromMilliseconds(200);

}  // namespace

// static
ConfirmQuitBubbleController* ConfirmQuitBubbleController::GetInstance() {
  return base::Singleton<ConfirmQuitBubbleController>::get();
}

ConfirmQuitBubbleController::ConfirmQuitBubbleController()
    : ConfirmQuitBubbleController(std::make_unique<ConfirmQuitBubble>(),
                                  std::make_unique<base::OneShotTimer>(),
                                  std::make_unique<gfx::SlideAnimation>(this)) {
}

ConfirmQuitBubbleController::ConfirmQuitBubbleController(
    std::unique_ptr<ConfirmQuitBubbleBase> bubble,
    std::unique_ptr<base::Timer> hide_timer,
    std::unique_ptr<gfx::SlideAnimation> animation)
    : view_(std::move(bubble)),
      state_(State::kWaiting),
      hide_timer_(std::move(hide_timer)),
      browser_hide_animation_(std::move(animation)) {
  browser_hide_animation_->SetSlideDuration(
      kWindowFadeOutDuration.InMilliseconds());
  BrowserList::AddObserver(this);
}

ConfirmQuitBubbleController::~ConfirmQuitBubbleController() {
  BrowserList::RemoveObserver(this);
}

bool ConfirmQuitBubbleController::HandleKeyboardEvent(
    const ui::Accelerator& accelerator) {
  if (state_ == State::kQuitting)
    return false;
  if (accelerator.key_code() == kAcceleratorKeyCode &&
      accelerator.modifiers() == kAcceleratorModifiers &&
      accelerator.key_state() == ui::Accelerator::KeyState::PRESSED &&
      !accelerator.IsRepeat()) {
    if (state_ == State::kWaiting) {
      state_ = State::kPressed;
      browser_ = BrowserList::GetInstance()->GetLastActive();
      view_->Show();
      hide_timer_->Start(FROM_HERE, kShowDuration, this,
                         &ConfirmQuitBubbleController::OnTimerElapsed);
    } else if (state_ == State::kReleased) {
      // The accelerator was pressed while the bubble was showing.  Consider
      // this a confirmation to quit.
      ConfirmQuit();
    }
    return true;
  }
  if (accelerator.key_code() == kAcceleratorKeyCode &&
      accelerator.key_state() == ui::Accelerator::KeyState::RELEASED) {
    if (state_ == State::kPressed)
      state_ = State::kReleased;
    else if (state_ == State::kConfirmed)
      Quit();
    return true;
  }
  return false;
}

void ConfirmQuitBubbleController::AnimationProgressed(
    const gfx::Animation* animation) {
  float opacity = static_cast<float>(animation->CurrentValueBetween(1.0, 0.0));
  for (Browser* browser : *BrowserList::GetInstance()) {
    BrowserView::GetBrowserViewForBrowser(browser)->GetWidget()->SetOpacity(
        opacity);
  }
}

void ConfirmQuitBubbleController::AnimationEnded(
    const gfx::Animation* animation) {
  AnimationProgressed(animation);
}

void ConfirmQuitBubbleController::OnBrowserNoLongerActive(Browser* browser) {
  if (browser != browser_ || state_ == State::kWaiting ||
      state_ == State::kQuitting) {
    return;
  }
  state_ = State::kWaiting;
  view_->Hide();
  hide_timer_->Stop();
  browser_hide_animation_->Hide();
}

void ConfirmQuitBubbleController::OnTimerElapsed() {
  if (state_ == State::kPressed) {
    // The accelerator was held down the entire time the bubble was showing.
    ConfirmQuit();
  } else if (state_ == State::kReleased) {
    state_ = State::kWaiting;
    view_->Hide();
  }
}

void ConfirmQuitBubbleController::ConfirmQuit() {
  DCHECK(state_ == State::kPressed || state_ == State::kReleased);
  state_ = State::kConfirmed;
  hide_timer_->Stop();
  browser_hide_animation_->Show();
}

void ConfirmQuitBubbleController::Quit() {
  DCHECK_EQ(state_, State::kConfirmed);
  state_ = State::kQuitting;
  if (quit_action_) {
    std::move(quit_action_).Run();
  } else {
    // Delay quitting because doing so destroys objects that may be used when
    // unwinding the stack.
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  base::BindOnce(chrome::Exit));
  }
}

void ConfirmQuitBubbleController::SetQuitActionForTest(
    base::OnceClosure quit_action) {
  quit_action_ = std::move(quit_action);
}
