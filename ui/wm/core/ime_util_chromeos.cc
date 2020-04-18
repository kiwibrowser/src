// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/core/ime_util_chromeos.h"

#include "base/command_line.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/base/ui_base_switches.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace wm {
namespace {

// Moves the window to ensure caret not in rect.
// Returns whether the window was moved or not.
void MoveWindowToEnsureCaretNotInRect(aura::Window* window,
                                      const gfx::Rect& rect_in_screen) {
  gfx::Rect original_window_bounds = window->GetBoundsInScreen();
  if (window->GetProperty(kVirtualKeyboardRestoreBoundsKey)) {
    original_window_bounds =
        *window->GetProperty(kVirtualKeyboardRestoreBoundsKey);
  }

  // Calculate vertical window shift.
  gfx::Rect rect_in_root_window = rect_in_screen;
  ::wm::ConvertRectFromScreen(window->GetRootWindow(), &rect_in_root_window);
  gfx::Rect bounds_in_root_window = original_window_bounds;
  ::wm::ConvertRectFromScreen(window->GetRootWindow(), &bounds_in_root_window);
  const int top_y =
      std::max(rect_in_root_window.y() - bounds_in_root_window.height(), 0);

  // No need to move the window up.
  if (top_y >= bounds_in_root_window.y())
    return;

  // Set restore bounds and move the window.
  window->SetProperty(kVirtualKeyboardRestoreBoundsKey,
                      new gfx::Rect(original_window_bounds));

  gfx::Rect new_bounds_in_root_window = bounds_in_root_window;
  new_bounds_in_root_window.set_y(top_y);
  window->SetBounds(new_bounds_in_root_window);
}

}  // namespace

DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(gfx::Rect,
                                   kVirtualKeyboardRestoreBoundsKey,
                                   nullptr);

void RestoreWindowBoundsOnClientFocusLost(aura::Window* window) {
  // Get restore bounds of the window
  gfx::Rect* vk_restore_bounds =
      window->GetProperty(kVirtualKeyboardRestoreBoundsKey);

  if (!vk_restore_bounds)
    return;

  // Restore the window bounds
  // TODO(yhanada): Don't move the window if a user has moved it while the
  // keyboard is shown.
  if (window->GetBoundsInScreen() != *vk_restore_bounds) {
    gfx::Rect original_bounds = *vk_restore_bounds;
    ::wm::ConvertRectFromScreen(window->GetRootWindow(), &original_bounds);
    window->SetBounds(original_bounds);
  }
  window->ClearProperty(wm::kVirtualKeyboardRestoreBoundsKey);
}

void EnsureWindowNotInRect(aura::Window* window,
                           const gfx::Rect& rect_in_screen) {
  gfx::Rect original_window_bounds = window->GetBoundsInScreen();
  if (window->GetProperty(wm::kVirtualKeyboardRestoreBoundsKey)) {
    original_window_bounds =
        *window->GetProperty(wm::kVirtualKeyboardRestoreBoundsKey);
  }

  gfx::Rect hidden_window_bounds_in_screen =
      gfx::IntersectRects(rect_in_screen, original_window_bounds);
  if (hidden_window_bounds_in_screen.IsEmpty()) {
    // The window isn't covered by the keyboard, restore the window position if
    // necessary.
    RestoreWindowBoundsOnClientFocusLost(window);
    return;
  }

  MoveWindowToEnsureCaretNotInRect(window, rect_in_screen);
}

}  // namespace wm
