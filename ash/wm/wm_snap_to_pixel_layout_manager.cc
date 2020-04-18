// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/wm_snap_to_pixel_layout_manager.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/wm/window_properties.h"
#include "ash/wm/window_util.h"
#include "ui/aura/window.h"

namespace ash {
namespace wm {

WmSnapToPixelLayoutManager::WmSnapToPixelLayoutManager() = default;

WmSnapToPixelLayoutManager::~WmSnapToPixelLayoutManager() = default;

// static
void WmSnapToPixelLayoutManager::InstallOnContainers(aura::Window* window) {
  for (aura::Window* child : window->children()) {
    if (child->id() < kShellWindowId_MinContainer ||
        child->id() > kShellWindowId_MaxContainer)  // not a container
      continue;
    if (child->GetProperty(kSnapChildrenToPixelBoundary)) {
      if (!child->layout_manager())
        child->SetLayoutManager(new WmSnapToPixelLayoutManager());
    } else {
      InstallOnContainers(child);
    }
  }
}

void WmSnapToPixelLayoutManager::OnWindowResized() {}

void WmSnapToPixelLayoutManager::OnWindowAddedToLayout(aura::Window* child) {}

void WmSnapToPixelLayoutManager::OnWillRemoveWindowFromLayout(
    aura::Window* child) {}

void WmSnapToPixelLayoutManager::OnWindowRemovedFromLayout(
    aura::Window* child) {}

void WmSnapToPixelLayoutManager::OnChildWindowVisibilityChanged(
    aura::Window* child,
    bool visible) {}

void WmSnapToPixelLayoutManager::SetChildBounds(
    aura::Window* child,
    const gfx::Rect& requested_bounds) {
  SetChildBoundsDirect(child, requested_bounds);
  wm::SnapWindowToPixelBoundary(child);
}

}  // namespace wm
}  // namespace ash
