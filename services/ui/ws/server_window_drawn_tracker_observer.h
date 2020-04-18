// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_SERVER_WINDOW_DRAWN_TRACKER_OBSERVER_H_
#define SERVICES_UI_WS_SERVER_WINDOW_DRAWN_TRACKER_OBSERVER_H_

namespace ui {

namespace ws {

class ServerWindow;

class ServerWindowDrawnTrackerObserver {
 public:
  // Invoked right before the drawn state changes. If |is_drawn| is false,
  // |ancestor| identifies where the change will occur. In the case of a remove,
  // |ancestor| is the parent of the window that will be removed (causing the
  // drawn state to change). In the case of visibility change, |ancestor| is the
  // parent of the window whose visibility will change.
  virtual void OnDrawnStateWillChange(ServerWindow* ancestor,
                                      ServerWindow* window,
                                      bool is_drawn) {}

  // Invoked when the drawn state changes. If |is_drawn| is false |ancestor|
  // identifies where the change occurred. In the case of a remove |ancestor| is
  // the parent of the window that was removed. In the case of a visibility
  // change |ancestor| is the parent of the window whose visibility changed.
  virtual void OnDrawnStateChanged(ServerWindow* ancestor,
                                   ServerWindow* window,
                                   bool is_drawn) {}

  // Invoked if the root will change as the result of a child of |ancestor|
  // being moved to a new root.
  virtual void OnRootWillChange(ServerWindow* ancestor, ServerWindow* window) {}

  // Invoked after the root changed. |ancestor| is the old ancestor that was the
  // old parent of the window (not necessarily |window|) that was moved.
  virtual void OnRootDidChange(ServerWindow* ancestor, ServerWindow* window) {}

 protected:
  virtual ~ServerWindowDrawnTrackerObserver() {}
};

}  // namespace ws

}  // namespace ui

#endif  // SERVICES_UI_WS_SERVER_WINDOW_DRAWN_TRACKER_OBSERVER_H_
