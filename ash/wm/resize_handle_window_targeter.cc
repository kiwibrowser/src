// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/resize_handle_window_targeter.h"

#include "ash/public/cpp/ash_constants.h"
#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ash/wm/window_state.h"
#include "ui/aura/window.h"
#include "ui/events/event.h"

namespace ash {

ResizeHandleWindowTargeter::ResizeHandleWindowTargeter(
    aura::Window* window,
    ImmersiveFullscreenController* controller)
    : window_(window), immersive_controller_(controller) {
  wm::WindowState* window_state = wm::GetWindowState(window_);
  OnPostWindowStateTypeChange(window_state, mojom::WindowStateType::DEFAULT);
  window_state->AddObserver(this);
  window_->AddObserver(this);
}

ResizeHandleWindowTargeter::~ResizeHandleWindowTargeter() {
  if (window_) {
    window_->RemoveObserver(this);
    wm::GetWindowState(window_)->RemoveObserver(this);
  }
}

void ResizeHandleWindowTargeter::OnPostWindowStateTypeChange(
    wm::WindowState* window_state,
    mojom::WindowStateType old_type) {
  if (window_state->IsMaximizedOrFullscreenOrPinned()) {
    frame_border_inset_ = gfx::Insets();
  } else {
    frame_border_inset_ =
        gfx::Insets(kResizeInsideBoundsSize, kResizeInsideBoundsSize,
                    kResizeInsideBoundsSize, kResizeInsideBoundsSize);
  }
}

void ResizeHandleWindowTargeter::OnWindowDestroying(aura::Window* window) {
  CHECK_EQ(window_, window);
  wm::GetWindowState(window_)->RemoveObserver(this);
  window_ = NULL;
}

bool ResizeHandleWindowTargeter::GetHitTestRects(
    aura::Window* window,
    gfx::Rect* hit_test_rect_mouse,
    gfx::Rect* hit_test_rect_touch) const {
  if (window == window_) {
    // Defer to the parent's targeter on whether |window_| should be able to
    // receive the event.
    ui::EventTarget* parent =
        static_cast<ui::EventTarget*>(window)->GetParentTarget();
    if (parent) {
      aura::WindowTargeter* targeter =
          static_cast<aura::WindowTargeter*>(parent->GetEventTargeter());
      if (targeter) {
        return targeter->GetHitTestRects(window, hit_test_rect_mouse,
                                         hit_test_rect_touch);
      }
    }
  }

  bool got_rects = WindowTargeter::GetHitTestRects(window, hit_test_rect_mouse,
                                                   hit_test_rect_touch);
  if (!got_rects || !window->parent() || window->parent() != window_)
    return got_rects;

  // If the event falls very close to the inside of the frame border, then
  // target the window itself, so that the window can be resized easily.
  // This is achieved by insetting the child (NativeViewHost).
  gfx::Insets mouse_insets;
  gfx::Insets touch_insets;
  touch_insets = mouse_insets = frame_border_inset_;

  // If the window is in immersive fullscreen, and top-of-window views are
  // not revealed, then touch events towards the top of the window
  // should not reach the child window so that touch gestures can be used
  // to reveal the top-of-windows views. This is needed because the child
  // window may consume touch events and prevent touch-scroll gesture from
  // being generated.
  if (immersive_controller_ && immersive_controller_->IsEnabled() &&
      !immersive_controller_->IsRevealed()) {
    touch_insets = gfx::Insets(
        ImmersiveFullscreenController::kImmersiveFullscreenTopEdgeInset, 0, 0,
        0);
  }
  hit_test_rect_mouse->Inset(mouse_insets);
  hit_test_rect_touch->Inset(touch_insets);
  return got_rects;
}

}  // namespace ash
