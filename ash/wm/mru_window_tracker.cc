// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/mru_window_tracker.h"

#include <algorithm>

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/shell.h"
#include "ash/wm/focus_rules.h"
#include "ash/wm/switchable_windows.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "base/bind.h"
#include "ui/aura/window.h"
#include "ui/wm/core/window_util.h"
#include "ui/wm/public/activation_client.h"

namespace ash {

namespace {

using CanActivateWindowPredicate = base::Callback<bool(aura::Window*)>;

bool CallCanActivate(aura::Window* window) {
  return ::wm::CanActivateWindow(window);
}

// Adds the windows that can be cycled through for the specified window id to
// |windows|.
void AddTrackedWindows(aura::Window* root,
                       int container_id,
                       MruWindowTracker::WindowList* windows) {
  aura::Window* container = root->GetChildById(container_id);
  const MruWindowTracker::WindowList children(container->children());
  windows->insert(windows->end(), children.begin(), children.end());
}

// Returns a list of windows ordered by their stacking order.
// If |mru_windows| is passed, these windows are moved to the front of the list.
// It uses the given |should_include_window_predicate| to determine whether to
// include a window in the returned list or not.
MruWindowTracker::WindowList BuildWindowListInternal(
    const std::list<aura::Window*>* mru_windows,
    const CanActivateWindowPredicate& should_include_window_predicate) {
  MruWindowTracker::WindowList windows;
  aura::Window* active_root = Shell::GetRootWindowForNewWindows();
  for (auto* window : Shell::GetAllRootWindows()) {
    if (window == active_root)
      continue;
    for (size_t i = 0; i < wm::kSwitchableWindowContainerIdsLength; ++i)
      AddTrackedWindows(window, wm::kSwitchableWindowContainerIds[i], &windows);
  }

  // Add windows in the active root windows last so that the topmost window
  // in the active root window becomes the front of the list.
  for (size_t i = 0; i < wm::kSwitchableWindowContainerIdsLength; ++i) {
    AddTrackedWindows(active_root, wm::kSwitchableWindowContainerIds[i],
                      &windows);
  }

  // Removes unfocusable windows.
  MruWindowTracker::WindowList::iterator itr = windows.begin();
  while (itr != windows.end()) {
    if (!should_include_window_predicate.Run(*itr))
      itr = windows.erase(itr);
    else
      ++itr;
  }

  // Put the windows in the mru_windows list at the head, if it's available.
  if (mru_windows) {
    // Iterate through the list backwards, so that we can move each window to
    // the front of the windows list as we find them.
    for (auto ix = mru_windows->rbegin(); ix != mru_windows->rend(); ++ix) {
      // Exclude windows in non-switchable containers and those which cannot
      // be activated.
      if (((*ix)->parent() && !wm::IsSwitchableContainer((*ix)->parent())) ||
          !should_include_window_predicate.Run(*ix)) {
        continue;
      }

      MruWindowTracker::WindowList::iterator window =
          std::find(windows.begin(), windows.end(), *ix);
      if (window != windows.end()) {
        windows.erase(window);
        windows.push_back(*ix);
      }
    }
  }

  // Window cycling expects the topmost window at the front of the list.
  std::reverse(windows.begin(), windows.end());

  return windows;
}

}  // namespace

//////////////////////////////////////////////////////////////////////////////
// MruWindowTracker, public:

MruWindowTracker::MruWindowTracker() : ignore_window_activations_(false) {
  Shell::Get()->activation_client()->AddObserver(this);
}

MruWindowTracker::~MruWindowTracker() {
  Shell::Get()->activation_client()->RemoveObserver(this);
  for (auto* window : mru_windows_)
    window->RemoveObserver(this);
}

MruWindowTracker::WindowList MruWindowTracker::BuildMruWindowList() const {
  return BuildWindowListInternal(&mru_windows_, base::Bind(&CallCanActivate));
}

MruWindowTracker::WindowList MruWindowTracker::BuildWindowListIgnoreModal()
    const {
  return BuildWindowListInternal(nullptr,
                                 base::Bind(&IsWindowConsideredActivatable));
}

MruWindowTracker::WindowList MruWindowTracker::BuildWindowForCycleList() const {
  MruWindowTracker::WindowList window_list = BuildMruWindowList();
  // Exclude windows:
  // - non user positionable windows, such as extension popups.
  // - windows being dragged
  // - the AppList window, which will hide as soon as cycling starts
  //   anyway. It doesn't make sense to count it as a "switchable" window, yet
  //   a lot of code relies on the MRU list returning the app window. If we
  //   don't manually remove it, the window cycling UI won't crash or misbehave,
  //   but there will be a flicker as the target window changes. Also exclude
  //   unselectable windows such as extension popups.
  auto window_is_ineligible = [](aura::Window* window) {
    wm::WindowState* state = wm::GetWindowState(window);
    return !state->IsUserPositionable() || state->is_dragged() ||
           window->GetRootWindow()
               ->GetChildById(kShellWindowId_AppListContainer)
               ->Contains(window) ||
           !window->GetProperty(kShowInOverviewKey);
  };
  window_list.erase(std::remove_if(window_list.begin(), window_list.end(),
                                   window_is_ineligible),
                    window_list.end());
  return window_list;
}

void MruWindowTracker::SetIgnoreActivations(bool ignore) {
  ignore_window_activations_ = ignore;

  // If no longer ignoring window activations, move currently active window
  // to front.
  if (!ignore)
    SetActiveWindow(wm::GetActiveWindow());
}

//////////////////////////////////////////////////////////////////////////////
// MruWindowTracker, private:

void MruWindowTracker::SetActiveWindow(aura::Window* active_window) {
  if (!active_window)
    return;

  std::list<aura::Window*>::iterator iter =
      std::find(mru_windows_.begin(), mru_windows_.end(), active_window);
  // Observe all newly tracked windows.
  if (iter == mru_windows_.end())
    active_window->AddObserver(this);
  else
    mru_windows_.erase(iter);
  mru_windows_.push_front(active_window);
}

void MruWindowTracker::OnWindowActivated(ActivationReason reason,
                                         aura::Window* gained_active,
                                         aura::Window* lost_active) {
  if (!ignore_window_activations_)
    SetActiveWindow(gained_active);
}

void MruWindowTracker::OnWindowDestroyed(aura::Window* window) {
  // It's possible for OnWindowActivated() to be called after
  // OnWindowDestroying(). This means we need to override OnWindowDestroyed()
  // else we may end up with a deleted window in |mru_windows_|.
  mru_windows_.remove(window);
  window->RemoveObserver(this);
}

}  // namespace ash
