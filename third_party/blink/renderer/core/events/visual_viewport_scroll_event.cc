// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/events/visual_viewport_scroll_event.h"

#include "third_party/blink/renderer/core/frame/use_counter.h"

namespace blink {

VisualViewportScrollEvent::~VisualViewportScrollEvent() = default;

VisualViewportScrollEvent::VisualViewportScrollEvent()
    : Event(EventTypeNames::scroll, Bubbles::kNo, Cancelable::kNo) {}

void VisualViewportScrollEvent::DoneDispatchingEventAtCurrentTarget() {
  UseCounter::Count(currentTarget()->GetExecutionContext(),
                    WebFeature::kVisualViewportScrollFired);
}

}  // namespace blink
