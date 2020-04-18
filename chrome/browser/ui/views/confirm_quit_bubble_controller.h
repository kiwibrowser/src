// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_CONFIRM_QUIT_BUBBLE_CONTROLLER_H_
#define CHROME_BROWSER_UI_VIEWS_CONFIRM_QUIT_BUBBLE_CONTROLLER_H_

#include <memory>

#include "base/macros.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "ui/gfx/animation/animation_delegate.h"

class ConfirmQuitBubbleBase;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace gfx {
class SlideAnimation;
}

namespace ui {
class Accelerator;
}

// Manages showing and hiding the confirm-to-quit bubble.  Requests Chrome to be
// closed if the quit accelerator is held down or pressed twice in succession.
class ConfirmQuitBubbleController : public gfx::AnimationDelegate,
                                    public BrowserListObserver {
 public:
  static ConfirmQuitBubbleController* GetInstance();

  ~ConfirmQuitBubbleController() override;

  // Returns true if the event was handled.
  bool HandleKeyboardEvent(const ui::Accelerator& accelerator);

 private:
  friend struct base::DefaultSingletonTraits<ConfirmQuitBubbleController>;
  friend class ConfirmQuitBubbleControllerTest;

  enum class State {
    // The accelerator has not been pressed.
    kWaiting,

    // The accelerator was pressed, but not yet released.
    kPressed,

    // The accelerator was pressed and released before the timer expired.
    kReleased,

    // The accelerator was either held down for the entire duration of the
    // timer, or was pressed a second time.  Either way, the accelerator is
    // currently held.
    kConfirmed,

    // The accelerator was released and Chrome is now quitting.
    kQuitting,
  };

  // |animation| is used to fade out all browser windows.
  ConfirmQuitBubbleController(std::unique_ptr<ConfirmQuitBubbleBase> bubble,
                              std::unique_ptr<base::Timer> hide_timer,
                              std::unique_ptr<gfx::SlideAnimation> animation);

  ConfirmQuitBubbleController();

  // gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

  // BrowserListObserver:
  void OnBrowserNoLongerActive(Browser* browser) override;

  void OnTimerElapsed();

  void ConfirmQuit();

  void Quit();

  void SetQuitActionForTest(base::OnceClosure quit_action);

  std::unique_ptr<ConfirmQuitBubbleBase> const view_;

  State state_;

  // The last active browser when the accelerator was pressed.
  Browser* browser_ = nullptr;

  std::unique_ptr<base::Timer> hide_timer_;

  std::unique_ptr<gfx::SlideAnimation> const browser_hide_animation_;

  base::OnceClosure quit_action_;

  DISALLOW_COPY_AND_ASSIGN(ConfirmQuitBubbleController);
};

#endif  // CHROME_BROWSER_UI_VIEWS_CONFIRM_QUIT_BUBBLE_CONTROLLER_H_
