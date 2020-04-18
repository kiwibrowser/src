// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WORKSPACE_CONTROLLER_TEST_API_H_
#define ASH_WM_WORKSPACE_CONTROLLER_TEST_API_H_

#include "ash/ash_export.h"
#include "ash/wm/workspace_controller.h"
#include "base/macros.h"

namespace ash {
class MultiWindowResizeController;
class WorkspaceEventHandler;

class ASH_EXPORT WorkspaceControllerTestApi {
 public:
  explicit WorkspaceControllerTestApi(WorkspaceController* controller);
  ~WorkspaceControllerTestApi();

  WorkspaceEventHandler* GetEventHandler();
  MultiWindowResizeController* GetMultiWindowResizeController();
  aura::Window* GetBackdropWindow();

 private:
  WorkspaceController* controller_;

  DISALLOW_COPY_AND_ASSIGN(WorkspaceControllerTestApi);
};

}  // namespace ash

#endif  // ASH_WM_WORKSPACE_CONTROLLER_TEST_API_H_
