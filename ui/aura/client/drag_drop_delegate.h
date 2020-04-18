// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_CLIENT_DRAG_DROP_DELEGATE_H_
#define UI_AURA_CLIENT_DRAG_DROP_DELEGATE_H_

#include "ui/aura/aura_export.h"

namespace ui {
class DropTargetEvent;
}

namespace aura {
class Window;
namespace client {

// Delegate interface for drag and drop actions on aura::Window.
class AURA_EXPORT DragDropDelegate {
 public:
  // OnDragEntered is invoked when the mouse enters this window during a drag &
  // drop session. This is immediately followed by an invocation of
  // OnDragUpdated, and eventually one of OnDragExited or OnPerformDrop.
  virtual void OnDragEntered(const ui::DropTargetEvent& event) = 0;

  // Invoked during a drag and drop session while the mouse is over the window.
  // This should return a bitmask of the DragDropTypes::DragOperation supported
  // based on the location of the event. Return 0 to indicate the drop should
  // not be accepted.
  virtual int OnDragUpdated(const ui::DropTargetEvent& event) = 0;

  // Invoked during a drag and drop session when the mouse exits the window, or
  // when the drag session was canceled and the mouse was over the window.
  virtual void OnDragExited() = 0;

  // Invoked during a drag and drop session when OnDragUpdated returns a valid
  // operation and the user release the mouse.
  virtual int OnPerformDrop(const ui::DropTargetEvent& event) = 0;

 protected:
  virtual ~DragDropDelegate() {}
};

AURA_EXPORT void SetDragDropDelegate(Window* window,
                                     DragDropDelegate* delegate);
AURA_EXPORT DragDropDelegate* GetDragDropDelegate(Window* window);

}  // namespace client
}  // namespace aura

#endif  // UI_AURA_CLIENT_DRAG_DROP_DELEGATE_H_
