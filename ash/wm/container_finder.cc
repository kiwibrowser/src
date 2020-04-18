// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/container_finder.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/wm/always_on_top_controller.h"
#include "ash/wm/root_window_finder.h"
#include "ash/wm/window_state.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/wm/core/window_util.h"

namespace ash {
namespace wm {
namespace {

aura::Window* FindContainerRoot(const gfx::Rect& bounds) {
  if (bounds == gfx::Rect())
    return Shell::GetRootWindowForNewWindows();
  return GetRootWindowMatching(bounds);
}

bool HasTransientParentWindow(const aura::Window* window) {
  const aura::Window* transient_parent = ::wm::GetTransientParent(window);
  return transient_parent &&
         transient_parent->type() != aura::client::WINDOW_TYPE_UNKNOWN;
}

aura::Window* GetSystemModalContainer(aura::Window* root,
                                      aura::Window* window) {
  aura::Window* transient_parent = ::wm::GetTransientParent(window);
  DCHECK_EQ(ui::MODAL_TYPE_SYSTEM,
            window->GetProperty(aura::client::kModalKey));

  // If screen lock is not active and user session is active,
  // all modal windows are placed into the normal modal container.
  // In case of missing transient parent (it could happen for alerts from
  // background pages) assume that the window belongs to user session.
  if (!Shell::Get()->session_controller()->IsUserSessionBlocked() ||
      !transient_parent) {
    return root->GetChildById(kShellWindowId_SystemModalContainer);
  }

  // Otherwise those that originate from LockScreen container and above are
  // placed in the screen lock modal container.
  int window_container_id = transient_parent->parent()->id();
  if (window_container_id < kShellWindowId_LockScreenContainer)
    return root->GetChildById(kShellWindowId_SystemModalContainer);
  return root->GetChildById(kShellWindowId_LockSystemModalContainer);
}

aura::Window* GetContainerFromAlwaysOnTopController(aura::Window* root,
                                                    aura::Window* window) {
  return RootWindowController::ForWindow(root)
      ->always_on_top_controller()
      ->GetContainer(window);
}

}  // namespace

aura::Window* GetContainerForWindow(aura::Window* window) {
  aura::Window* parent = window->parent();
  // The first parent with an explicit shell window ID is the container.
  while (parent && parent->id() == kShellWindowId_Invalid)
    parent = parent->parent();
  return parent;
}

aura::Window* GetDefaultParent(aura::Window* window, const gfx::Rect& bounds) {
  aura::Window* target_root = nullptr;
  aura::Window* transient_parent = ::wm::GetTransientParent(window);
  if (transient_parent) {
    // Transient window should use the same root as its transient parent.
    target_root = transient_parent->GetRootWindow();
  } else {
    target_root = FindContainerRoot(bounds);
  }

  switch (window->type()) {
    case aura::client::WINDOW_TYPE_NORMAL:
    case aura::client::WINDOW_TYPE_POPUP:
      if (window->GetProperty(aura::client::kModalKey) == ui::MODAL_TYPE_SYSTEM)
        return GetSystemModalContainer(target_root, window);
      if (HasTransientParentWindow(window))
        return GetContainerForWindow(transient_parent);
      return GetContainerFromAlwaysOnTopController(target_root, window);
    case aura::client::WINDOW_TYPE_CONTROL:
      return target_root->GetChildById(
          kShellWindowId_UnparentedControlContainer);
    case aura::client::WINDOW_TYPE_PANEL:
      if (window->GetProperty(kPanelAttachedKey))
        return target_root->GetChildById(kShellWindowId_PanelContainer);
      return GetContainerFromAlwaysOnTopController(target_root, window);
    case aura::client::WINDOW_TYPE_MENU:
      return target_root->GetChildById(kShellWindowId_MenuContainer);
    case aura::client::WINDOW_TYPE_TOOLTIP:
      return target_root->GetChildById(
          kShellWindowId_DragImageAndTooltipContainer);
    default:
      NOTREACHED() << "Window " << window->id() << " has unhandled type "
                   << window->type();
      break;
  }
  return nullptr;
}

aura::Window::Windows GetContainersFromAllRootWindows(
    int container_id,
    aura::Window* priority_root) {
  aura::Window::Windows containers;
  for (aura::Window* root : Shell::GetAllRootWindows()) {
    aura::Window* container = root->GetChildById(container_id);
    if (!container)
      continue;

    if (priority_root && priority_root->Contains(container))
      containers.insert(containers.begin(), container);
    else
      containers.push_back(container);
  }
  return containers;
}

}  // namespace wm
}  // namespace ash
