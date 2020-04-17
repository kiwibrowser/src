// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp_base/with_destruction_callback.h"

#include "platform/api/logging.h"

namespace openscreen {

WithDestructionCallback::WithDestructionCallback() = default;

WithDestructionCallback::~WithDestructionCallback() {
  if (destruction_callback_function_) {
    destruction_callback_function_(destruction_callback_state_);
  }
}

void WithDestructionCallback::SetDestructionCallback(
    WithDestructionCallback::DestructionCallbackFunctionPointer function,
    void* state) {
  OSP_DCHECK(!destruction_callback_function_);
  destruction_callback_function_ = function;
  destruction_callback_state_ = state;
}

}  // namespace openscreen
