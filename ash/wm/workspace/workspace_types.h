// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WORKSPACE_WORKSPACE_TYPES_H_
#define ASH_WM_WORKSPACE_WORKSPACE_TYPES_H_

namespace ash {
namespace wm {

// Enumeration of the possible window states.
enum WorkspaceWindowState {
  // There's a full screen window.
  WORKSPACE_WINDOW_STATE_FULL_SCREEN,

  // There's a maximized window.
  WORKSPACE_WINDOW_STATE_MAXIMIZED,

  // At least one window overlaps the shelf.
  WORKSPACE_WINDOW_STATE_WINDOW_OVERLAPS_SHELF,

  // None of the windows are fullscreen, maximized or touch the shelf.
  WORKSPACE_WINDOW_STATE_DEFAULT,
};

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_WORKSPACE_WORKSPACE_TYPES_H_
