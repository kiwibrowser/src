// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WORKSPACE_WORKSPACE_EVENT_HANDLER_MASH_H_
#define ASH_WM_WORKSPACE_WORKSPACE_EVENT_HANDLER_MASH_H_

#include "ash/wm/workspace/workspace_event_handler.h"
#include "base/macros.h"

namespace aura {
class Window;
}

namespace ash {

// TODO(sky): investigate if can use aura version.
class WorkspaceEventHandlerMash : public WorkspaceEventHandler {
 public:
  explicit WorkspaceEventHandlerMash(aura::Window* workspace_window);
  ~WorkspaceEventHandlerMash() override;

  // Returns the WorkspaceEventHandlerMash associated with |window|, or null
  // if |window| is not the workspace window.
  static WorkspaceEventHandlerMash* Get(aura::Window* window);

  // Returns the window associated with the workspace.
  aura::Window* workspace_window() { return workspace_window_; }

 private:
  aura::Window* workspace_window_;

  DISALLOW_COPY_AND_ASSIGN(WorkspaceEventHandlerMash);
};

}  // namespace ash

#endif  // ASH_WM_WORKSPACE_WORKSPACE_EVENT_HANDLER_MASH_H_
