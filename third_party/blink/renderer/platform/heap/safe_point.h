// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_SAFE_POINT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_SAFE_POINT_H_

#include "base/macros.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"
#include "third_party/blink/renderer/platform/wtf/threading_primitives.h"

namespace blink {

class SafePointScope final {
  STACK_ALLOCATED();

 public:
  explicit SafePointScope(BlinkGC::StackState stack_state,
                          ThreadState* state = ThreadState::Current())
      : state_(state) {
    if (state_) {
      state_->EnterSafePoint(stack_state, this);
    }
  }

  ~SafePointScope() {
    if (state_)
      state_->LeaveSafePoint();
  }

 private:
  ThreadState* state_;

  DISALLOW_COPY_AND_ASSIGN(SafePointScope);
};

}  // namespace blink

#endif
