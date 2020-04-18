// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/keyboard_layout_manager.h"

#include "ui/compositor/layer_animator.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_util.h"

namespace keyboard {

// Overridden from aura::LayoutManager
void KeyboardLayoutManager::OnWindowResized() {
  if (contents_window_) {
    gfx::Rect container_bounds = controller_->GetContainerWindow()->bounds();
    // Always align container window and keyboard window.
    SetChildBounds(contents_window_, container_bounds);
  }
}

void KeyboardLayoutManager::OnWindowAddedToLayout(aura::Window* child) {
  DCHECK(!contents_window_);
  contents_window_ = child;
  controller_->GetContainerWindow()->SetBounds(gfx::Rect());
}

void KeyboardLayoutManager::SetChildBounds(aura::Window* child,
                                           const gfx::Rect& requested_bounds) {
  DCHECK(child == contents_window_);
  TRACE_EVENT0("vk", "KeyboardLayoutSetChildBounds");

  // Request to change the bounds of the contents window
  // should change the container window first. Then the contents window is
  // resized and covers the container window. Note the contents' bound is only
  // set in OnWindowResized.

  aura::Window* root_window =
      controller_->GetContainerWindow()->GetRootWindow();

  // If the keyboard has been deactivated, this reference will be null.
  if (!root_window)
    return;

  DisplayUtil display_util;
  const display::Display& display =
      display_util.GetNearestDisplayToWindow(root_window);
  const gfx::Vector2d display_offset =
      display.bounds().origin().OffsetFromOrigin();

  const gfx::Rect new_bounds =
      controller_->AdjustSetBoundsRequest(display.bounds(),
                                          requested_bounds + display_offset) -
      display_offset;

  // Containar bounds should only be reset when the contents window bounds
  // actually change. Otherwise it interrupts the initial animation of showing
  // the keyboard. Described in crbug.com/356753.
  gfx::Rect old_bounds = contents_window_->GetTargetBounds();

  aura::Window::ConvertRectToTarget(contents_window_, root_window, &old_bounds);
  if (new_bounds == old_bounds)
    return;

  SetChildBoundsDirect(contents_window_, gfx::Rect(new_bounds.size()));
  const bool contents_loaded =
      old_bounds.height() == 0 && new_bounds.height() > 0;

  controller_->SetContainerBounds(new_bounds, contents_loaded);
}

}  // namespace keyboard
