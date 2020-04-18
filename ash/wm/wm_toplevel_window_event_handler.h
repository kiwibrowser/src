// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WM_TOPLEVEL_WINDOW_EVENT_HANDLER_H_
#define ASH_WM_WM_TOPLEVEL_WINDOW_EVENT_HANDLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/display/window_tree_host_manager.h"
#include "base/callback.h"
#include "base/macros.h"
#include "ui/aura/window_observer.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/wm/public/window_move_client.h"

namespace aura {
class Window;
}

namespace ui {
class KeyEvent;
class LocatedEvent;
class MouseEvent;
class GestureEvent;
}

namespace ash {
namespace mojom {
enum class WindowStateType;
}

namespace wm {

// WmToplevelWindowEventHandler handles dragging and resizing of top level
// windows. WmToplevelWindowEventHandler is forwarded events, such as from an
// EventHandler.
class ASH_EXPORT WmToplevelWindowEventHandler
    : public WindowTreeHostManager::Observer,
      public aura::WindowObserver {
 public:
  // Describes what triggered ending the drag.
  enum class DragResult {
    // The drag successfully completed.
    SUCCESS,

    REVERT,

    // The underlying window was destroyed while the drag is in process.
    WINDOW_DESTROYED
  };
  using EndClosure = base::Callback<void(DragResult)>;

  WmToplevelWindowEventHandler();
  ~WmToplevelWindowEventHandler() override;

  void OnKeyEvent(ui::KeyEvent* event);
  void OnMouseEvent(ui::MouseEvent* event, aura::Window* target);
  void OnGestureEvent(ui::GestureEvent* event, aura::Window* target);

  // Attempts to start a drag if one is not already in progress. Returns true if
  // successful. |end_closure| is run when the drag completes. |end_closure| is
  // not run if the drag does not start.
  bool AttemptToStartDrag(aura::Window* window,
                          const gfx::Point& point_in_parent,
                          int window_component,
                          ::wm::WindowMoveSource source,
                          const EndClosure& end_closure);

  // If there is a drag in progress it is reverted, otherwise does nothing.
  void RevertDrag();

  // Returns true if there is a drag in progress.
  bool is_drag_in_progress() const { return window_resizer_.get() != nullptr; }

  // Returns the window that is currently handling gesture events.
  aura::Window* gesture_target() { return gesture_target_; }

 private:
  class ScopedWindowResizer;

  // Completes or reverts the drag if one is in progress. Returns true if a
  // drag was completed or reverted.
  bool CompleteDrag(DragResult result);

  void HandleMousePressed(aura::Window* target, ui::MouseEvent* event);
  void HandleMouseReleased(aura::Window* target, ui::MouseEvent* event);

  // Called during a drag to resize/position the window.
  void HandleDrag(aura::Window* target, ui::LocatedEvent* event);

  // Called during mouse moves to update window resize shadows.
  void HandleMouseMoved(aura::Window* target, ui::LocatedEvent* event);

  // Called for mouse exits to hide window resize shadows.
  void HandleMouseExited(aura::Window* target, ui::LocatedEvent* event);

  // Called when mouse capture is lost.
  void HandleCaptureLost(ui::LocatedEvent* event);

  // Sets |window|'s state type to |new_state_type|. Called after the drag has
  // been completed for fling gestures.
  void SetWindowStateTypeFromGesture(aura::Window* window,
                                     mojom::WindowStateType new_state_type);

  // Invoked from ScopedWindowResizer if the window is destroyed.
  void ResizerWindowDestroyed();

  // WindowTreeHostManager::Observer:
  void OnDisplayConfigurationChanging() override;

  // aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;

  // Update the gesture target.
  void UpdateGestureTarget(aura::Window* window);

  // The hittest result for the first finger at the time that it initially
  // touched the screen. |first_finger_hittest_| is one of ui/base/hit_test.h
  int first_finger_hittest_;

  // The point for the first finger at the time that it initially touched the
  // screen.
  gfx::Point first_finger_touch_point_;

  // The window bounds when the drag was started. When a window is minimized,
  // maximized or snapped via a swipe/fling gesture, the restore bounds should
  // be set to the bounds of the window when the drag was started.
  gfx::Rect pre_drag_window_bounds_;

  // Is a window move/resize in progress because of gesture events?
  bool in_gesture_drag_ = false;

  aura::Window* gesture_target_ = nullptr;

  std::unique_ptr<ScopedWindowResizer> window_resizer_;

  EndClosure end_closure_;

  DISALLOW_COPY_AND_ASSIGN(WmToplevelWindowEventHandler);
};

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_WM_TOPLEVEL_WINDOW_EVENT_HANDLER_H_
