// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WORKSPACE_CONTROLLER_H_
#define ASH_WM_WORKSPACE_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/wm/workspace/workspace_types.h"
#include "base/macros.h"
#include "ui/aura/window_observer.h"

namespace ash {

class BackdropDelegate;
class WorkspaceEventHandler;
class WorkspaceLayoutManager;

// WorkspaceController acts as a central place that ties together all the
// various workspace pieces.
class ASH_EXPORT WorkspaceController : public aura::WindowObserver {
 public:
  // Installs WorkspaceLayoutManager on |viewport|.
  explicit WorkspaceController(aura::Window* viewport);
  ~WorkspaceController() override;

  // Returns the current window state.
  wm::WorkspaceWindowState GetWindowState() const;

  // Starts the animation that occurs on first login.
  void DoInitialAnimation();

  // Add a delegate which adds a backdrop behind the top window of the default
  // workspace.
  void SetBackdropDelegate(std::unique_ptr<BackdropDelegate> delegate);

  WorkspaceLayoutManager* layout_manager() { return layout_manager_; }

 private:
  friend class WorkspaceControllerTestApi;

  // aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;

  aura::Window* viewport_;
  std::unique_ptr<WorkspaceEventHandler> event_handler_;

  // Owned by |viewport_|.
  WorkspaceLayoutManager* layout_manager_;

  DISALLOW_COPY_AND_ASSIGN(WorkspaceController);
};

}  // namespace ash

#endif  // ASH_WM_WORKSPACE_CONTROLLER_H_
