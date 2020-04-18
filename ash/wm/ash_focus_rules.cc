// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/ash_focus_rules.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/wm/container_finder.h"
#include "ash/wm/focus_rules.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/window_state.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/events/event.h"

namespace ash {
namespace wm {
namespace {

bool BelongsToContainerWithEqualOrGreaterId(const aura::Window* window,
                                            int container_id) {
  for (; window; window = window->parent()) {
    if (window->id() >= container_id)
      return true;
  }
  return false;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// AshFocusRules, public:

AshFocusRules::AshFocusRules() = default;

AshFocusRules::~AshFocusRules() = default;

////////////////////////////////////////////////////////////////////////////////
// AshFocusRules, ::wm::FocusRules:

bool AshFocusRules::IsToplevelWindow(aura::Window* window) const {
  return ash::IsToplevelWindow(window);
}

bool AshFocusRules::SupportsChildActivation(aura::Window* window) const {
  return ash::IsActivatableShellWindowId(window->id());
}

bool AshFocusRules::IsWindowConsideredVisibleForActivation(
    aura::Window* window) const {
  return ash::IsWindowConsideredVisibleForActivation(window);
}

bool AshFocusRules::CanActivateWindow(aura::Window* window) const {
  // Clearing activation is always permissible.
  if (!window)
    return true;

  if (!BaseFocusRules::CanActivateWindow(window)) {
    return false;
  }

  int modal_container_id = Shell::GetOpenSystemModalWindowContainerId();
  if (modal_container_id >= 0)
    return BelongsToContainerWithEqualOrGreaterId(window, modal_container_id);

  return true;
}

bool AshFocusRules::CanFocusWindow(aura::Window* window,
                                   const ui::Event* event) const {
  if (!window)
    return true;

  if (event && (event->IsMouseEvent() || event->IsGestureEvent()) &&
      !window->GetProperty(aura::client::kActivateOnPointerKey)) {
    return false;
  }

  return BaseFocusRules::CanFocusWindow(window, event);
}

aura::Window* AshFocusRules::GetNextActivatableWindow(
    aura::Window* ignore) const {
  DCHECK(ignore);

  // Start from the container of the most-recently-used window. If the list of
  // MRU windows is empty, then start from the container of the window that just
  // lost focus |ignore|.
  MruWindowTracker* mru = Shell::Get()->mru_window_tracker();
  aura::Window::Windows windows = mru->BuildMruWindowList();
  aura::Window* starting_window = windows.empty() ? ignore : windows[0];

  // Look for windows to focus in |starting_window|'s container. If none are
  // found, we look in all the containers in front of |starting_window|'s
  // container, then all behind.
  int starting_container_index = 0;
  aura::Window* root = starting_window->GetRootWindow();
  if (!root)
    root = Shell::GetRootWindowForNewWindows();
  int container_count = static_cast<int>(kNumActivatableShellWindowIds);
  for (int i = 0; i < container_count; i++) {
    aura::Window* container =
        Shell::GetContainer(root, kActivatableShellWindowIds[i]);
    if (container && container->Contains(starting_window)) {
      starting_container_index = i;
      break;
    }
  }

  aura::Window* window = nullptr;
  for (int i = starting_container_index; !window && i < container_count; i++)
    window = GetTopmostWindowToActivateForContainerIndex(i, ignore);
  if (!window && starting_container_index > 0) {
    for (int i = starting_container_index - 1; !window && i >= 0; i--)
      window = GetTopmostWindowToActivateForContainerIndex(i, ignore);
  }
  return window;
}

////////////////////////////////////////////////////////////////////////////////
// AshFocusRules, private:

aura::Window* AshFocusRules::GetTopmostWindowToActivateForContainerIndex(
    int index,
    aura::Window* ignore) const {
  aura::Window* window = nullptr;
  aura::Window* root = ignore ? ignore->GetRootWindow() : nullptr;
  aura::Window::Windows containers =
      GetContainersFromAllRootWindows(kActivatableShellWindowIds[index], root);
  for (aura::Window* container : containers) {
    window = GetTopmostWindowToActivateInContainer(container, ignore);
    if (window)
      return window;
  }
  return window;
}

aura::Window* AshFocusRules::GetTopmostWindowToActivateInContainer(
    aura::Window* container,
    aura::Window* ignore) const {
  for (aura::Window::Windows::const_reverse_iterator i =
           container->children().rbegin();
       i != container->children().rend(); ++i) {
    WindowState* window_state = GetWindowState(*i);
    if (*i != ignore && window_state->CanActivate() &&
        !window_state->IsMinimized())
      return *i;
  }
  return nullptr;
}

}  // namespace wm
}  // namespace ash
