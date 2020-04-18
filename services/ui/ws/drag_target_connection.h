// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_DRAG_TARGET_CONNECTION_H_
#define SERVICES_UI_WS_DRAG_TARGET_CONNECTION_H_

#include <string>

#include "base/bind.h"
#include "base/containers/flat_map.h"
#include "ui/gfx/geometry/point.h"

namespace ui {
namespace ws {

class ServerWindow;

// An abstract connection which can respond to drag/drop target requests.
//
// The methods in this class send drag and drop messages to the client and
// return their results through the passed in callback. From the point of view
// of the client, the lifecycle of receiving messages are in the following
// order:
class DragTargetConnection {
 public:
  virtual ~DragTargetConnection() {}

  // On the first time that the pointer enters a window offered by this
  // connection, we send this start message with the |mime_data| of the
  // drag so that we only send this data over the connection once. We send the
  // mime data first because clients may read the payload at any time,
  // including during the enter message.
  //
  // (As an optimization, on the server side, we don't send this message to the
  // source window of the drag; the client library has already done the
  // equivalent in ui::WindowDropTarget to minimize the load of inter-process
  // communication.)
  virtual void PerformOnDragDropStart(
      const base::flat_map<std::string, std::vector<uint8_t>>& mime_data) = 0;

  // Next, on each time that the mouse cursor moves from one |window| to
  // another, we send a DragEnter message. The value returned by |callback| is
  // a bitmask of drop operations that can be performed at this location, in
  // terms of the ui::mojom::kDropEffect{None,Move,Copy,Link} constants.
  virtual void PerformOnDragEnter(
      const ServerWindow* window,
      uint32_t key_state,
      const gfx::Point& cursor_offset,
      uint32_t effect_bitmask,
      const base::Callback<void(uint32_t)>& callback) = 0;

  // For each mouse move after the initial DragEnter message, we call DragOver
  // to change the mouse location and to update what drop operations can be
  // performed.
  virtual void PerformOnDragOver(
      const ServerWindow* window,
      uint32_t key_state,
      const gfx::Point& cursor_offset,
      uint32_t effect_bitmask,
      const base::Callback<void(uint32_t)>& callback) = 0;

  // If the mouse cursor leaves |window|, send an DragLeave message.
  virtual void PerformOnDragLeave(const ServerWindow* window) = 0;

  // If the user releases the pointer over a window, send a DragDrop message,
  // which signals that the client should complete the drag. The return value
  // is which operation was performed; a non-zero callback value means the drag
  // was accepted and completed.
  virtual void PerformOnCompleteDrop(
      const ServerWindow* window,
      uint32_t key_state,
      const gfx::Point& cursor_offset,
      uint32_t effect_bitmask,
      const base::Callback<void(uint32_t)>& callback) = 0;

  // Finally, regardless of which window accepted (or rejected) the drop, we
  // send a done message to each connection that we sent a start message
  // to. This message is used to clear cached data from the drag.
  //
  // (Again, we don't send this message to the source window as we didn't send
  // a DragStart message; the client library handles the equivalent at its
  // layer.)
  virtual void PerformOnDragDropDone() = 0;
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_DRAG_TARGET_CONNECTION_H_
