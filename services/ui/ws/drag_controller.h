// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_DRAG_CONTROLLER_H_
#define SERVICES_UI_WS_DRAG_CONTROLLER_H_

#include <map>
#include <memory>
#include <set>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "services/ui/common/types.h"
#include "services/ui/public/interfaces/cursor/cursor.mojom.h"
#include "services/ui/ws/ids.h"
#include "services/ui/ws/server_window_observer.h"
#include "services/ui/ws/server_window_tracker.h"

namespace gfx {
class Point;
}

namespace ui {
class PointerEvent;

namespace ws {

namespace test {
class DragControllerTestApi;
}

class DragCursorUpdater;
class DragSource;
class DragTargetConnection;

// A single ui::mojom::kDropEffect operation.
using DropEffect = uint32_t;

// A bitmask of ui::mojom::kDropEffect operations.
using DropEffectBitmask = uint32_t;

// Represents all the data around the current ongoing drag operation.
//
// There should only be one instance of this class per userid. The
// WindowManagerState's EventDispatcher creates and owns this instance.
class DragController : public ServerWindowObserver {
 public:
  DragController(
      DragCursorUpdater* cursor_updater,
      DragSource* source,
      ServerWindow* source_window,
      DragTargetConnection* source_connection,
      int32_t drag_pointer,
      const base::flat_map<std::string, std::vector<uint8_t>>& mime_data,
      DropEffectBitmask drag_operations);
  ~DragController() override;

  const ui::CursorData& current_cursor() const { return current_cursor_; }

  // Cancels the current drag, ie, due to the user pressing Escape.
  void Cancel();

  // Responds to a pointer move/release event. Returns true if the event was
  // handled by the drag.
  bool DispatchPointerEvent(const ui::PointerEvent& event,
                            ServerWindow* current_target);

  void OnWillDestroyDragTargetConnection(DragTargetConnection* connection);

 private:
  friend class test::DragControllerTestApi;
  enum class OperationType { NONE, ENTER, OVER, LEAVE, DROP };
  struct Operation;
  struct WindowState;

  // Notifies all windows we messaged that the drag is finished, and then tell
  // |source| the result.
  void MessageDragCompleted(bool success, DropEffect action_taken);

  // Returns the number of events on |window|. A value of 1 means that there's
  // a single event outstanding that we're waiting for a response from the
  // client, all values over 1 are queued and will be dispatched when the event
  // in the front of the queue gets a response.
  size_t GetSizeOfQueueForWindow(ServerWindow* window);

  // Sets |current_target_window_| to |current_target|, making sure that we add
  // and release ServerWindow observers correctly.
  void SetCurrentTargetWindow(ServerWindow* current_target);

  // Updates the possible cursor effects for |window|. |bitmask| is a
  // bitmask of the current valid drag operations.
  void SetWindowDropOperations(ServerWindow* window, DropEffectBitmask bitmask);

  // Returns the cursor for the window |bitmask|, adjusted for types that the
  // drag source allows.
  ui::CursorData CursorForEffectBitmask(DropEffectBitmask bitmask);

  // Ensure that |window| has an entry in |window_state_| and that we're an
  // observer.
  void EnsureWindowObserved(ServerWindow* window);

  void QueueOperation(ServerWindow* window,
                      OperationType type,
                      uint32_t event_flags,
                      const gfx::Point& screen_position);
  void DispatchOperation(ServerWindow* window, WindowState* state);
  void OnRespondToOperation(ServerWindow* window);

  // Callback methods. |tracker| contains the window being queried and is null
  // if the window was destroyed while waiting for client.
  void OnDragStatusCompleted(std::unique_ptr<ServerWindowTracker> tracker,
                             DropEffectBitmask bitmask);
  void OnDragDropCompleted(std::unique_ptr<ServerWindowTracker> tracker,
                           DropEffect action);

  // ServerWindowObserver:
  void OnWindowDestroying(ServerWindow* window) override;

  static std::string ToString(OperationType type);

  // Our owner.
  DragSource* source_;

  // Object to notify about all cursor changes.
  DragCursorUpdater* cursor_updater_;

  // A bit-field of acceptable drag operations offered by the source.
  const DropEffectBitmask drag_operations_;

  // Only act on pointer events that meet this id.
  const int32_t drag_pointer_id_;

  // The current mouse cursor during the drag.
  ui::CursorData current_cursor_;

  // Sending OnDragOver() to our |source_| destroys us; there is a period where
  // we have to continue to exist, but not process any more pointer events.
  bool waiting_for_final_drop_response_ = false;

  ServerWindow* source_window_;
  ServerWindow* current_target_window_ = nullptr;

  // The target connection that |source_window_| is part of.
  DragTargetConnection* source_connection_;

  // A list of the offered mime types.
  base::flat_map<std::string, std::vector<uint8_t>> mime_data_;

  // We need to keep track of state on a per window basis. A window being in
  // this map means that we're observing it. WindowState also keeps track of
  // what type of operation we're waiting for a response from the window's
  // client, along with a queued operation to send when we get a reply.
  std::map<ServerWindow*, WindowState> window_state_;

  // A set of DragTargetConnections* which have received the
  // PerformOnDragMimeTypes() call.
  std::set<DragTargetConnection*> called_on_drag_mime_types_;

  base::WeakPtrFactory<DragController> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DragController);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_DRAG_CONTROLLER_H_
