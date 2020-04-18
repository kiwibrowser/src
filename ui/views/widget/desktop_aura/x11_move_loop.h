// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_X11_MOVE_LOOP_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_X11_MOVE_LOOP_H_

#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/x/x11_types.h"

namespace views {

// Runs a nested run loop and grabs the mouse. This is used to implement
// dragging.
class X11MoveLoop {
 public:
  virtual ~X11MoveLoop() {}

  // Runs the nested run loop. While the mouse is grabbed, use |cursor| as
  // the mouse cursor. Returns true if the move-loop is completed successfully.
  // If the pointer-grab fails, or the move-loop is canceled by the user (e.g.
  // by pressing escape), then returns false.
  virtual bool RunMoveLoop(aura::Window* window, gfx::NativeCursor cursor) = 0;

  // Updates the cursor while the move loop is running.
  virtual void UpdateCursor(gfx::NativeCursor cursor) = 0;

  // Ends the move loop that's currently in progress.
  virtual void EndMoveLoop() = 0;
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_X11_MOVE_LOOP_H_
