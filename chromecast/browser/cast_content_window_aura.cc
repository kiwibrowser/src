// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_content_window_aura.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "chromecast/graphics/cast_window_manager.h"
#include "content/public/browser/web_contents.h"
#include "ui/aura/window.h"

namespace chromecast {
namespace shell {

class TouchBlocker : public ui::EventHandler, public aura::WindowObserver {
 public:
  TouchBlocker(aura::Window* window, bool activated)
      : window_(window), activated_(activated) {
    DCHECK(window_);
    window_->AddObserver(this);
    if (activated_) {
      window_->AddPreTargetHandler(this);
    }
  }

  ~TouchBlocker() override {
    if (window_) {
      window_->RemoveObserver(this);
      if (activated_) {
        window_->RemovePreTargetHandler(this);
      }
    }
  }

  void Activate(bool activate) {
    if (!window_ || activate == activated_) {
      return;
    }

    if (activate) {
      window_->AddPreTargetHandler(this);
    } else {
      window_->RemovePreTargetHandler(this);
    }

    activated_ = activate;
  }

 private:
  // Overriden from ui::EventHandler.
  void OnTouchEvent(ui::TouchEvent* touch) override {
    if (activated_) {
      touch->SetHandled();
    }
  }

  // Overriden from aura::WindowObserver.
  void OnWindowDestroyed(aura::Window* window) override { window_ = nullptr; }

  aura::Window* window_;
  bool activated_;

  DISALLOW_COPY_AND_ASSIGN(TouchBlocker);
};

// static
std::unique_ptr<CastContentWindow> CastContentWindow::Create(
    CastContentWindow::Delegate* delegate,
    bool is_headless,
    bool enable_touch_input) {
  return base::WrapUnique(
      new CastContentWindowAura(delegate, enable_touch_input));
}

CastContentWindowAura::CastContentWindowAura(
    CastContentWindow::Delegate* delegate,
    bool is_touch_enabled)
    : back_gesture_dispatcher_(
          std::make_unique<CastBackGestureDispatcher>(delegate)),
      is_touch_enabled_(is_touch_enabled) {}

CastContentWindowAura::~CastContentWindowAura() {
  if (window_manager_)
    window_manager_->RemoveSideSwipeGestureHandler(this);
}

void CastContentWindowAura::CreateWindowForWebContents(
    content::WebContents* web_contents,
    CastWindowManager* window_manager,
    bool is_visible,
    CastWindowManager::WindowId z_order,
    VisibilityPriority visibility_priority) {
  DCHECK(web_contents);
  window_manager_ = window_manager;
  DCHECK(window_manager_);
  gfx::NativeView window = web_contents->GetNativeView();
  window_manager_->SetWindowId(window, z_order);
  window_manager_->AddWindow(window);
  window_manager_->AddSideSwipeGestureHandler(this);

  touch_blocker_ = std::make_unique<TouchBlocker>(window, !is_touch_enabled_);

  if (is_visible) {
    window->Show();
  } else {
    window->Hide();
  }
}

void CastContentWindowAura::EnableTouchInput(bool enabled) {
  if (touch_blocker_) {
    touch_blocker_->Activate(!enabled);
  }
}

void CastContentWindowAura::RequestVisibility(
    VisibilityPriority visibility_priority){};

void CastContentWindowAura::RequestMoveOut(){};

bool CastContentWindowAura::CanHandleSwipe(CastSideSwipeOrigin swipe_origin) {
  return back_gesture_dispatcher_->CanHandleSwipe(swipe_origin);
}

void CastContentWindowAura::HandleSideSwipeBegin(
    CastSideSwipeOrigin swipe_origin,
    const gfx::Point& touch_location) {
  back_gesture_dispatcher_->HandleSideSwipeBegin(swipe_origin, touch_location);
}

void CastContentWindowAura::HandleSideSwipeContinue(
    CastSideSwipeOrigin swipe_origin,
    const gfx::Point& touch_location) {
  back_gesture_dispatcher_->HandleSideSwipeContinue(swipe_origin,
                                                    touch_location);
}

}  // namespace shell
}  // namespace chromecast
