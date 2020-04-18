// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/root_window_layout_manager.h"

#include "ui/aura/window.h"
#include "ui/aura/window_tracker.h"

namespace ash {

namespace {

// Resize all container windows that RootWindowLayoutManager is responsible for.
// That includes all container windows up to three depth except that top level
// window which has a delegate. We cannot simply check top level window, because
// we need to skip other windows without a delegate, such as ScreenDimmer
// windows.
// TODO(wutao): The above logic is error prone. Consider using a Shell window id
// to indentify such a container.
void ResizeWindow(const aura::Window::Windows& children,
                  const gfx::Rect& fullscreen_bounds,
                  int depth) {
  if (depth > 2)
    return;
  const int child_depth = depth + 1;
  aura::WindowTracker children_tracker(children);
  while (!children_tracker.windows().empty()) {
    aura::Window* child = children_tracker.Pop();
    if (child->GetToplevelWindow())
      continue;
    child->SetBounds(fullscreen_bounds);
    ResizeWindow(child->children(), fullscreen_bounds, child_depth);
  }
}

}  // namespace

namespace wm {

////////////////////////////////////////////////////////////////////////////////
// RootWindowLayoutManager, public:

RootWindowLayoutManager::RootWindowLayoutManager(aura::Window* owner)
    : owner_(owner) {}

RootWindowLayoutManager::~RootWindowLayoutManager() = default;

////////////////////////////////////////////////////////////////////////////////
// RootWindowLayoutManager, aura::LayoutManager implementation:

void RootWindowLayoutManager::OnWindowResized() {
  ResizeWindow(owner_->children(), gfx::Rect(owner_->bounds().size()), 0);
}

void RootWindowLayoutManager::OnWindowAddedToLayout(aura::Window* child) {}

void RootWindowLayoutManager::OnWillRemoveWindowFromLayout(
    aura::Window* child) {}

void RootWindowLayoutManager::OnWindowRemovedFromLayout(aura::Window* child) {}

void RootWindowLayoutManager::OnChildWindowVisibilityChanged(
    aura::Window* child,
    bool visible) {}

void RootWindowLayoutManager::SetChildBounds(
    aura::Window* child,
    const gfx::Rect& requested_bounds) {
  SetChildBoundsDirect(child, requested_bounds);
}

}  // namespace wm
}  // namespace ash
