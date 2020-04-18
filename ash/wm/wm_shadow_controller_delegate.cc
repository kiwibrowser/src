// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/wm_shadow_controller_delegate.h"

#include "ash/shell.h"
#include "ash/wm/overview/window_selector.h"
#include "ash/wm/overview/window_selector_controller.h"
#include "ash/wm/splitview/split_view_controller.h"
#include "ui/aura/window.h"

namespace ash {

WmShadowControllerDelegate::WmShadowControllerDelegate() = default;

WmShadowControllerDelegate::~WmShadowControllerDelegate() = default;

bool WmShadowControllerDelegate::ShouldShowShadowForWindow(
    const aura::Window* window) {
  SplitViewController* split_view_controller =
      Shell::Get()->split_view_controller();
  if (!split_view_controller)
    return true;
  // Hide the shadow if it is one of the splitscreen snapped windows.
  if (window == split_view_controller->left_window() ||
      window == split_view_controller->right_window()) {
    return false;
  }
  // Hide the shadow while we are in overview mode.
  WindowSelectorController* window_selector_controller =
      Shell::Get()->window_selector_controller();
  if (!window_selector_controller || !window_selector_controller->IsSelecting())
    return true;
  WindowSelector* window_selector =
      window_selector_controller->window_selector();
  DCHECK(window_selector);
  return window_selector->IsShuttingDown() ||
         !window_selector->IsWindowInOverview(window);
}

}  // namespace ash