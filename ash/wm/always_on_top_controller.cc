// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/always_on_top_controller.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/wm/workspace/workspace_layout_manager.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"

namespace ash {

AlwaysOnTopController::AlwaysOnTopController(aura::Window* viewport)
    : always_on_top_container_(viewport) {
  DCHECK_NE(kShellWindowId_DefaultContainer, viewport->id());
  always_on_top_container_->SetLayoutManager(
      new WorkspaceLayoutManager(viewport));
  // Container should be empty.
  DCHECK(always_on_top_container_->children().empty());
  always_on_top_container_->AddObserver(this);
}

AlwaysOnTopController::~AlwaysOnTopController() {
  if (always_on_top_container_)
    always_on_top_container_->RemoveObserver(this);
}

aura::Window* AlwaysOnTopController::GetContainer(aura::Window* window) const {
  DCHECK(always_on_top_container_);
  if (window->GetProperty(aura::client::kAlwaysOnTopKey))
    return always_on_top_container_;
  return always_on_top_container_->GetRootWindow()->GetChildById(
      kShellWindowId_DefaultContainer);
}

// TODO(rsadam@): Refactor so that this cast is unneeded.
WorkspaceLayoutManager* AlwaysOnTopController::GetLayoutManager() const {
  return static_cast<WorkspaceLayoutManager*>(
      always_on_top_container_->layout_manager());
}

void AlwaysOnTopController::SetLayoutManagerForTest(
    std::unique_ptr<WorkspaceLayoutManager> layout_manager) {
  always_on_top_container_->SetLayoutManager(layout_manager.release());
}

void AlwaysOnTopController::OnWindowHierarchyChanged(
    const HierarchyChangeParams& params) {
  if (params.old_parent == always_on_top_container_)
    params.target->RemoveObserver(this);
  else if (params.new_parent == always_on_top_container_)
    params.target->AddObserver(this);
}

void AlwaysOnTopController::OnWindowPropertyChanged(aura::Window* window,
                                                    const void* key,
                                                    intptr_t old) {
  if (window != always_on_top_container_ &&
      key == aura::client::kAlwaysOnTopKey) {
    DCHECK(window->type() == aura::client::WINDOW_TYPE_NORMAL ||
           window->type() == aura::client::WINDOW_TYPE_POPUP);
    aura::Window* container = GetContainer(window);
    if (window->parent() != container)
      container->AddChild(window);
  }
}

void AlwaysOnTopController::OnWindowDestroying(aura::Window* window) {
  if (window == always_on_top_container_) {
    always_on_top_container_->RemoveObserver(this);
    always_on_top_container_ = nullptr;
  }
}

}  // namespace ash
