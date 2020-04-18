// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/focus_rules.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/shell_delegate.h"
#include "ash/wm/window_state.h"
#include "ui/aura/window.h"
#include "ui/wm/public/activation_delegate.h"

namespace ash {

bool IsToplevelWindow(aura::Window* window) {
  DCHECK(window);
  // The window must in a valid hierarchy.
  if (!window->GetRootWindow())
    return false;

  // The window must exist within a container that supports activation.
  // The window cannot be blocked by a modal transient.
  return IsActivatableShellWindowId(window->parent()->id());
}

bool IsWindowConsideredActivatable(aura::Window* window) {
  DCHECK(window);
  // Only toplevel windows can be activated.
  if (!IsToplevelWindow(window))
    return false;

  if (!IsWindowConsideredVisibleForActivation(window))
    return false;

  if (::wm::GetActivationDelegate(window) &&
      !::wm::GetActivationDelegate(window)->ShouldActivate()) {
    return false;
  }

  return window->CanFocus();
}

bool IsWindowConsideredVisibleForActivation(aura::Window* window) {
  DCHECK(window);
  // If the |window| doesn't belong to the current active user and also doesn't
  // show for the current active user, then it should not be activated.
  if (!Shell::Get()->shell_delegate()->CanShowWindowForUser(window))
    return false;

  if (window->IsVisible())
    return true;

  // Minimized windows are hidden in their minimized state, but they can always
  // be activated.
  if (wm::GetWindowState(window)->IsMinimized())
    return true;

  if (!window->TargetVisibility())
    return false;

  const int parent_shell_window_id = window->parent()->id();
  return parent_shell_window_id == kShellWindowId_DefaultContainer ||
         parent_shell_window_id == kShellWindowId_LockScreenContainer;
}

}  // namespace ash
