// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_DRAG_SOURCE_H_
#define SERVICES_UI_WS_DRAG_SOURCE_H_

namespace ui {
namespace ws {

class DragTargetConnection;
class ServerWindow;

// An interface implemented by the object that caused a DragController to
// exist, and which acts as a delegate to it.
class DragSource {
 public:
  virtual ~DragSource() {}

  // Called during a drag operation when the mouse cursor moves.
  virtual void OnDragMoved(const gfx::Point& location) = 0;

  // Called when a drag operation is completed. |success| is true when a target
  // window signaled the successful completion of the drag, false in all other
  // cases where a drag was aborted at any step in the process. |action_taken|
  // is one of the kDropEffect constants in window_tree_constants.mojom.
  virtual void OnDragCompleted(bool success, uint32_t action_taken) = 0;

  // Returns the client connection that created |window|.
  virtual DragTargetConnection* GetDragTargetForWindow(
      const ServerWindow* window) = 0;
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_DRAG_SOURCE_H_
