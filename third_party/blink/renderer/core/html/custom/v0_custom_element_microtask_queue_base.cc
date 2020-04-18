// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/custom/v0_custom_element_microtask_queue_base.h"

#include "third_party/blink/renderer/core/html/custom/v0_custom_element_processing_stack.h"

namespace blink {

void V0CustomElementMicrotaskQueueBase::Dispatch() {
  CHECK(!in_dispatch_);
  in_dispatch_ = true;
  DoDispatch();
  in_dispatch_ = false;
}

void V0CustomElementMicrotaskQueueBase::Trace(blink::Visitor* visitor) {
  visitor->Trace(queue_);
}

#if !defined(NDEBUG)
void V0CustomElementMicrotaskQueueBase::Show(unsigned indent) {
  for (const auto& step : queue_) {
    if (step)
      step->Show(indent);
    else
      fprintf(stderr, "%*snull\n", indent, "");
  }
}
#endif

}  // namespace blink
