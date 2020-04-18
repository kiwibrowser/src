// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/drag_controller.h"

#include <utility>

#include "base/logging.h"
#include "services/ui/public/interfaces/cursor/cursor.mojom.h"
#include "services/ui/ws/drag_cursor_updater.h"
#include "services/ui/ws/drag_source.h"
#include "services/ui/ws/drag_target_connection.h"
#include "services/ui/ws/server_window.h"
#include "ui/base/cursor/cursor.h"

namespace ui {
namespace ws {

struct DragController::Operation {
  OperationType type;
  uint32_t event_flags;
  gfx::Point screen_position;
};

struct DragController::WindowState {
  // Set to true once we've observed the ServerWindow* that is the key to this
  // instance in |window_state_|.
  bool observed = false;

  // If we're waiting for a response, this is the type of message. NONE means
  // there's no outstanding
  OperationType waiting_on_reply = OperationType::NONE;

  // The operation that we'll send off if |waiting_on_reply| isn't NONE.
  Operation queued_operation = {OperationType::NONE, 0, gfx::Point()};

  // The current set of operations that this window accepts. This gets updated
  // on each return message.
  DropEffectBitmask bitmask = 0u;
};

DragController::DragController(
    DragCursorUpdater* cursor_updater,
    DragSource* source,
    ServerWindow* source_window,
    DragTargetConnection* source_connection,
    int32_t drag_pointer,
    const base::flat_map<std::string, std::vector<uint8_t>>& mime_data,
    DropEffectBitmask drag_operations)
    : source_(source),
      cursor_updater_(cursor_updater),
      drag_operations_(drag_operations),
      drag_pointer_id_(drag_pointer),
      current_cursor_(ui::CursorType::kNoDrop),
      source_window_(source_window),
      source_connection_(source_connection),
      mime_data_(mime_data),
      weak_factory_(this) {
  SetCurrentTargetWindow(nullptr);
  EnsureWindowObserved(source_window_);
}

DragController::~DragController() {
  for (auto& pair : window_state_) {
    if (pair.second.observed)
      pair.first->RemoveObserver(this);
  }
}

void DragController::Cancel() {
  MessageDragCompleted(false, ui::mojom::kDropEffectNone);
  // |this| may be deleted now.
}

bool DragController::DispatchPointerEvent(const ui::PointerEvent& event,
                                          ServerWindow* current_target) {
  DVLOG(2) << "DragController dispatching pointer event at "
           << event.location().ToString();
  uint32_t event_flags =
      event.flags() &
      (ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN);
  gfx::Point screen_position = event.location();

  if (waiting_for_final_drop_response_) {
    // If we're waiting on a target window to respond to the final drag drop
    // call, don't process any more pointer events.
    DVLOG(1) << "Ignoring event because we're waiting for final drop response";
    return false;
  }

  if (event.pointer_details().id != drag_pointer_id_) {
    DVLOG(1) << "Ignoring event from different pointer "
             << event.pointer_details().id;
    return false;
  }

  // If |current_target| doesn't accept drags, walk its hierarchy up until we
  // find one that does (or set to nullptr at the top of the tree).
  while (current_target && !current_target->can_accept_drops())
    current_target = current_target->parent();

  if (current_target) {
    // If we're non-null, we're about to use |current_target| in some
    // way. Ensure that we receive notifications that this window has gone
    // away.
    EnsureWindowObserved(current_target);
  }

  source_->OnDragMoved(screen_position);

  if (current_target && current_target == current_target_window_ &&
      event.type() != ET_POINTER_UP) {
    QueueOperation(current_target, OperationType::OVER, event_flags,
                   screen_position);
  } else if (current_target != current_target_window_) {
    if (current_target_window_) {
      QueueOperation(current_target_window_, OperationType::LEAVE, event_flags,
                     screen_position);
    }

    if (current_target) {
      // TODO(erg): If we have a queued LEAVE operation, does this turn into a
      // noop?
      QueueOperation(current_target, OperationType::ENTER, event_flags,
                     screen_position);
    }

    SetCurrentTargetWindow(current_target);
  } else if (event.type() != ET_POINTER_UP) {
    DVLOG(1) << "Performing no action for pointer event at "
             << screen_position.ToString()
             << "! current_target=" << current_target;
  }

  if (event.type() == ET_POINTER_UP) {
    if (current_target) {
      QueueOperation(current_target, OperationType::DROP, event_flags,
                     screen_position);
      waiting_for_final_drop_response_ = true;
    } else {
      // The pointer was released over no window or a window that doesn't
      // accept drags.
      MessageDragCompleted(false, ui::mojom::kDropEffectNone);
    }
  }

  return true;
}

void DragController::OnWillDestroyDragTargetConnection(
    DragTargetConnection* connection) {
  called_on_drag_mime_types_.erase(connection);
}

void DragController::MessageDragCompleted(bool success,
                                          DropEffect action_taken) {
  DVLOG(1) << "Drag Completed: success=" << success
           << ", action_taken=" << action_taken;
  for (DragTargetConnection* connection : called_on_drag_mime_types_)
    connection->PerformOnDragDropDone();
  called_on_drag_mime_types_.clear();

  source_->OnDragCompleted(success, action_taken);
  // |this| may be deleted now.
}

size_t DragController::GetSizeOfQueueForWindow(ServerWindow* window) {
  auto it = window_state_.find(window);
  if (it == window_state_.end())
    return 0u;
  if (it->second.waiting_on_reply == OperationType::NONE)
    return 0u;
  if (it->second.queued_operation.type == OperationType::NONE)
    return 1u;
  return 2u;
}

void DragController::SetWindowDropOperations(ServerWindow* window,
                                             DropEffectBitmask bitmask) {
  WindowState& state = window_state_[window];
  state.bitmask = bitmask;

  if (current_target_window_ == window) {
    current_cursor_ = CursorForEffectBitmask(bitmask);
    cursor_updater_->OnDragCursorUpdated();
  }
}

ui::CursorData DragController::CursorForEffectBitmask(
    DropEffectBitmask bitmask) {
  DropEffectBitmask combined = bitmask & drag_operations_;
  return combined == ui::mojom::kDropEffectNone
             ? ui::CursorData(ui::CursorType::kNoDrop)
             : ui::CursorData(ui::CursorType::kCopy);
}

void DragController::SetCurrentTargetWindow(ServerWindow* current_target) {
  current_target_window_ = current_target;

  if (current_target_window_) {
    // Immediately set the cursor to the last known set of operations (which
    // could be none).
    WindowState& state = window_state_[current_target_window_];
    current_cursor_ = CursorForEffectBitmask(state.bitmask);
  } else {
    // Can't drop in empty areas.
    current_cursor_ = ui::CursorData(ui::CursorType::kNoDrop);
  }

  cursor_updater_->OnDragCursorUpdated();
}

void DragController::EnsureWindowObserved(ServerWindow* window) {
  if (!window)
    return;

  WindowState& state = window_state_[window];
  if (!state.observed) {
    state.observed = true;
    window->AddObserver(this);
  }
}

void DragController::QueueOperation(ServerWindow* window,
                                    OperationType type,
                                    uint32_t event_flags,
                                    const gfx::Point& screen_position) {
  DVLOG(2) << "Queueing operation " << ToString(type) << " to " << window;

  // If this window doesn't have the mime data, send it.
  DragTargetConnection* connection = source_->GetDragTargetForWindow(window);
  if (connection != source_connection_ &&
      !base::ContainsKey(called_on_drag_mime_types_, connection)) {
    connection->PerformOnDragDropStart(mime_data_);
    called_on_drag_mime_types_.insert(connection);
  }

  WindowState& state = window_state_[window];
  // Set the queued operation to the incoming.
  state.queued_operation = {type, event_flags, screen_position};

  if (state.waiting_on_reply == OperationType::NONE) {
    // Send the operation immediately.
    DispatchOperation(window, &state);
  }
}

void DragController::DispatchOperation(ServerWindow* target,
                                       WindowState* state) {
  DragTargetConnection* connection = source_->GetDragTargetForWindow(target);

  DCHECK_EQ(OperationType::NONE, state->waiting_on_reply);
  Operation& op = state->queued_operation;
  switch (op.type) {
    case OperationType::NONE: {
      // NONE case to silence the compiler.
      NOTREACHED();
      break;
    }
    case OperationType::ENTER: {
      std::unique_ptr<ServerWindowTracker> tracker =
          std::make_unique<ServerWindowTracker>();
      tracker->Add(target);
      connection->PerformOnDragEnter(
          target, op.event_flags, op.screen_position, drag_operations_,
          base::Bind(&DragController::OnDragStatusCompleted,
                     weak_factory_.GetWeakPtr(), base::Passed(&tracker)));
      state->waiting_on_reply = OperationType::ENTER;
      break;
    }
    case OperationType::OVER: {
      std::unique_ptr<ServerWindowTracker> tracker =
          std::make_unique<ServerWindowTracker>();
      tracker->Add(target);
      connection->PerformOnDragOver(
          target, op.event_flags, op.screen_position, drag_operations_,
          base::Bind(&DragController::OnDragStatusCompleted,
                     weak_factory_.GetWeakPtr(), base::Passed(&tracker)));
      state->waiting_on_reply = OperationType::OVER;
      break;
    }
    case OperationType::LEAVE: {
      connection->PerformOnDragLeave(target);
      state->waiting_on_reply = OperationType::NONE;
      break;
    }
    case OperationType::DROP: {
      std::unique_ptr<ServerWindowTracker> tracker =
          std::make_unique<ServerWindowTracker>();
      tracker->Add(target);
      connection->PerformOnCompleteDrop(
          target, op.event_flags, op.screen_position, drag_operations_,
          base::Bind(&DragController::OnDragDropCompleted,
                     weak_factory_.GetWeakPtr(), base::Passed(&tracker)));
      state->waiting_on_reply = OperationType::DROP;
      break;
    }
  }

  state->queued_operation = {OperationType::NONE, 0, gfx::Point()};
}

void DragController::OnRespondToOperation(ServerWindow* window) {
  WindowState& state = window_state_[window];
  DCHECK_NE(OperationType::NONE, state.waiting_on_reply);
  state.waiting_on_reply = OperationType::NONE;
  if (state.queued_operation.type != OperationType::NONE)
    DispatchOperation(window, &state);
}

void DragController::OnDragStatusCompleted(
    std::unique_ptr<ServerWindowTracker> tracker,
    DropEffectBitmask bitmask) {
  if (tracker->windows().empty()) {
    // The window has been deleted and its queue is empty.
    return;
  }

  // We must remove the completed item.
  OnRespondToOperation(*(tracker->windows().begin()));
  SetWindowDropOperations(*(tracker->windows().begin()), bitmask);
}

void DragController::OnDragDropCompleted(
    std::unique_ptr<ServerWindowTracker> tracker,
    DropEffect action) {
  if (tracker->windows().empty()) {
    // The window has been deleted after we sent the drop message. It's really
    // hard to recover from this so just signal to the source that our drag
    // failed.
    MessageDragCompleted(false, ui::mojom::kDropEffectNone);
    return;
  }

  OnRespondToOperation(*(tracker->windows().begin()));
  MessageDragCompleted(action != 0u, action);
}

void DragController::OnWindowDestroying(ServerWindow* window) {
  auto it = window_state_.find(window);
  if (it != window_state_.end()) {
    window->RemoveObserver(this);
    window_state_.erase(it);
  }

  if (current_target_window_ == window)
    SetCurrentTargetWindow(nullptr);

  if (source_window_ == window) {
    source_window_ = nullptr;
    // Our source window is being deleted, fail the drag.
    MessageDragCompleted(false, ui::mojom::kDropEffectNone);
  }
}

// static
std::string DragController::ToString(OperationType type) {
  switch (type) {
    case OperationType::NONE:
      return "NONE";
    case OperationType::ENTER:
      return "ENTER";
    case OperationType::OVER:
      return "OVER";
    case OperationType::LEAVE:
      return "LEAVE";
    case OperationType::DROP:
      return "DROP";
  }
  NOTREACHED();
  return std::string();
}

}  // namespace ws
}  // namespace ui
