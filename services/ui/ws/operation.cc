// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/operation.h"

#include "services/ui/ws/window_server.h"
#include "services/ui/ws/window_tree.h"

namespace ui {
namespace ws {

Operation::Operation(WindowTree* tree,
                     WindowServer* window_server,
                     OperationType operation_type)
    : window_server_(window_server),
      source_tree_id_(tree->id()),
      operation_type_(operation_type) {
  DCHECK(operation_type != OperationType::NONE);
  // Tell the window server about the operation currently in flight. The window
  // server uses this information to suppress certain calls out to clients.
  window_server_->PrepareForOperation(this);
}

Operation::~Operation() {
  window_server_->FinishOperation();
}

}  // namespace ws
}  // namespace ui
