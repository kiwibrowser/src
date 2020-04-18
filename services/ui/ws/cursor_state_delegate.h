// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_CURSOR_STATE_DELEGATE_H_
#define SERVICES_UI_WS_CURSOR_STATE_DELEGATE_H_

namespace ui {

namespace ws {

// Interface used for the CursorState object to message back to the window
// manager.
class CursorStateDelegate {
 public:
  virtual void OnCursorTouchVisibleChanged(bool enabled) = 0;

 protected:
  virtual ~CursorStateDelegate() {}
};

}  // namespace ws

}  // namespace ui

#endif  // SERVICES_UI_WS_CURSOR_STATE_DELEGATE_H_
