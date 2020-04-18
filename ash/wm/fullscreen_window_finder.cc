// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/fullscreen_window_finder.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/wm/switchable_windows.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/wm/core/window_util.h"

namespace ash {
namespace wm {

aura::Window* GetWindowForFullscreenMode(aura::Window* context) {
  aura::Window* topmost_window = nullptr;
  aura::Window* active_window = GetActiveWindow();
  if (active_window &&
      active_window->GetRootWindow() == context->GetRootWindow() &&
      IsSwitchableContainer(active_window->parent())) {
    // Use the active window when it is on the current root window to determine
    // the fullscreen state to allow temporarily using a panel (which is always
    // above the default container) while a fullscreen window is open. We only
    // use the active window when in a switchable container as the launcher
    // should not exit fullscreen mode.
    topmost_window = active_window;
  } else {
    // Otherwise, use the topmost window on the root window's default container
    // when there is no active window on this root window.
    const std::vector<aura::Window*>& windows =
        context->GetRootWindow()
            ->GetChildById(kShellWindowId_DefaultContainer)
            ->children();
    for (auto iter = windows.rbegin(); iter != windows.rend(); ++iter) {
      if (GetWindowState(*iter)->IsUserPositionable() &&
          (*iter)->layer()->GetTargetVisibility()) {
        topmost_window = *iter;
        break;
      }
    }
  }
  while (topmost_window) {
    if (GetWindowState(topmost_window)->IsFullscreen() ||
        GetWindowState(topmost_window)->IsPinned())
      return topmost_window;
    topmost_window = ::wm::GetTransientParent(topmost_window);
  }
  return nullptr;
}

}  // namespace wm
}  // namespace ash
