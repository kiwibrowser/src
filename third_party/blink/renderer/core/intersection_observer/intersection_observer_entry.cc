// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/intersection_observer/intersection_observer_entry.h"

#include "third_party/blink/renderer/core/dom/element.h"

namespace blink {

IntersectionObserverEntry::IntersectionObserverEntry(
    DOMHighResTimeStamp time,
    double intersection_ratio,
    const FloatRect& bounding_client_rect,
    const FloatRect* root_bounds,
    const FloatRect& intersection_rect,
    bool is_intersecting,
    bool is_visible,
    Element* target)
    : time_(time),
      intersection_ratio_(intersection_ratio),
      bounding_client_rect_(
          DOMRectReadOnly::FromFloatRect(bounding_client_rect)),
      root_bounds_(root_bounds ? DOMRectReadOnly::FromFloatRect(*root_bounds)
                               : nullptr),
      intersection_rect_(DOMRectReadOnly::FromFloatRect(intersection_rect)),
      target_(target),
      is_intersecting_(is_intersecting),
      is_visible_(is_visible)

{}

void IntersectionObserverEntry::Trace(blink::Visitor* visitor) {
  visitor->Trace(bounding_client_rect_);
  visitor->Trace(root_bounds_);
  visitor->Trace(intersection_rect_);
  visitor->Trace(target_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
