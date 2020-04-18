// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_MOVE_EVENT_HANDLER_H_
#define ASH_WM_MOVE_EVENT_HANDLER_H_

#include <memory>

#include "ash/wm/wm_toplevel_window_event_handler.h"
#include "base/macros.h"
#include "ui/aura/window_observer.h"
#include "ui/events/event_handler.h"

namespace aura {
class Window;
class WindowManagerClient;
}  // namespace aura

namespace ui {
class CancelModeEvent;
}

namespace ash {

class WorkspaceEventHandlerMash;

// EventHandler attached to windows that may be dragged and/or resized. This
// forwards to WmToplevelWindowEventHandler to handle the actual drag/resize.
//
// TODO(sky): rename this class to better reflect that it handles redirecting
// events in addition to drag.
class MoveEventHandler : public ui::EventHandler, public aura::WindowObserver {
 public:
  MoveEventHandler(aura::WindowManagerClient* window_manager_client,
                   aura::Window* window);
  ~MoveEventHandler() override;

  // Retrieves the MoveEventHandler for an existing aura::Window.
  static MoveEventHandler* GetForWindow(aura::Window* window);

  // Attempts to start a drag if one is not already in progress. This passes
  // the call to the underlying WmToplevelWindowEventHandler. After the drag
  // completes, |end_closure| will be called to return whether the drag was
  // successfully completed.
  void AttemptToStartDrag(
      const gfx::Point& point_in_parent,
      int window_component,
      ::wm::WindowMoveSource source,
      const base::Callback<void(bool success)>& end_closure);

  // Returns whether we're in a drag.
  bool IsDragInProgress();

  // Reverts a manually started drag started with AttemptToStartDrag().
  void RevertDrag();

 private:
  // Removes observer and EventHandler.
  void Detach();

  // Returns the WorkspaceEventHandlerMash, or null if the window is not in a
  // workspace.
  WorkspaceEventHandlerMash* GetWorkspaceEventHandlerMash();

  // Overridden from ui::EventHandler:
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  void OnCancelMode(ui::CancelModeEvent* event) override;

  // Overridden from aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;

  aura::Window* window_;
  aura::WindowManagerClient* window_manager_client_;
  wm::WmToplevelWindowEventHandler toplevel_window_event_handler_;

  DISALLOW_COPY_AND_ASSIGN(MoveEventHandler);
};

}  // namespace ash

#endif  // ASH_WM_MOVE_EVENT_HANDLER_H_
