// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/tablet_mode/tablet_mode_backdrop_delegate_impl.h"
#include "ash/shell.h"
#include "ash/wm/splitview/split_view_controller.h"
#include "ash/wm/window_util.h"

namespace ash {

namespace {

// returns true if window |upper| is stacked above window |lower| in the window
// stacking order.
bool IsWindowAbove(aura::Window* upper, aura::Window* lower) {
  if (!upper || !lower || upper == lower || upper->parent() != lower->parent())
    return false;

  const aura::Window::Windows windows = upper->parent()->children();
  auto upper_i = std::find(windows.begin(), windows.end(), upper);
  auto lower_i = std::find(windows.begin(), windows.end(), lower);
  return upper_i > lower_i;
}

}  // namespace

TabletModeBackdropDelegateImpl::TabletModeBackdropDelegateImpl() = default;

TabletModeBackdropDelegateImpl::~TabletModeBackdropDelegateImpl() = default;

bool TabletModeBackdropDelegateImpl::HasBackdrop(aura::Window* window) {
  if (!Shell::Get()->IsSplitViewModeActive())
    return true;

  // If the split view mode is active, we should place the backdrop below the
  // snapped window whose window stacking order is lower.
  SplitViewController* split_view_controller =
      Shell::Get()->split_view_controller();
  aura::Window* left_window = split_view_controller->left_window();
  aura::Window* right_window = split_view_controller->right_window();
  if (window == left_window && IsWindowAbove(window, right_window))
    return false;
  if (window == right_window && IsWindowAbove(window, left_window))
    return false;
  return true;
}

}  // namespace ash
