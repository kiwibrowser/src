// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_DRAG_CURSOR_UPDATER_H_
#define SERVICES_UI_WS_DRAG_CURSOR_UPDATER_H_

namespace ui {
namespace ws {

// An interface for the DragController to signal that the cursor has changed.
class DragCursorUpdater {
 public:
  virtual void OnDragCursorUpdated() = 0;

 protected:
  virtual ~DragCursorUpdater() {}
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_DRAG_CURSOR_UPDATER_H_
