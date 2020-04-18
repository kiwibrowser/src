// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_TOPLEVEL_WINDOW_EVENT_HANDLER_H_
#define ASH_WM_TOPLEVEL_WINDOW_EVENT_HANDLER_H_

#include "ash/ash_export.h"
#include "ash/wm/wm_toplevel_window_event_handler.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/events/event_handler.h"
#include "ui/wm/public/window_move_client.h"

namespace aura {
class Window;
}

namespace base {
class RunLoop;
}

namespace ash {

class ASH_EXPORT ToplevelWindowEventHandler : public ui::EventHandler,
                                              public ::wm::WindowMoveClient {
 public:
  ToplevelWindowEventHandler();
  ~ToplevelWindowEventHandler() override;

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

  // Attempts to start a drag if one is not already in progress. Returns true if
  // successful. |end_closure| is run when the drag completes.
  // If the event handler is handing the gesture stream, it will use the touch
  // movement.
  bool AttemptToStartDrag(
      aura::Window* window,
      const gfx::Point& point_in_parent,
      int window_component,
      const wm::WmToplevelWindowEventHandler::EndClosure& end_closure);

  // Overridden form wm::WindowMoveClient:
  ::wm::WindowMoveResult RunMoveLoop(
      aura::Window* source,
      const gfx::Vector2d& drag_offset,
      ::wm::WindowMoveSource move_source) override;
  void EndMoveLoop() override;

  aura::Window* gesture_target() {
    return wm_toplevel_window_event_handler_.gesture_target();
  }

 private:
  // Callback from WmToplevelWindowEventHandler once the drag completes.
  void OnDragCompleted(
      wm::WmToplevelWindowEventHandler::DragResult* result_return_value,
      base::RunLoop* run_loop,
      wm::WmToplevelWindowEventHandler::DragResult result);

  wm::WmToplevelWindowEventHandler wm_toplevel_window_event_handler_;

  // Are we running a nested run loop from RunMoveLoop().
  bool in_move_loop_ = false;

  base::WeakPtrFactory<ToplevelWindowEventHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ToplevelWindowEventHandler);
};

}  // namespace ash

#endif  // ASH_WM_TOPLEVEL_WINDOW_EVENT_HANDLER_H_
