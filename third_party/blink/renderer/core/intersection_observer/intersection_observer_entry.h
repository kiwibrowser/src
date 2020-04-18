// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVER_ENTRY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVER_ENTRY_H_

#include "third_party/blink/renderer/core/dom/dom_high_res_time_stamp.h"
#include "third_party/blink/renderer/core/geometry/dom_rect_read_only.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class Element;

class IntersectionObserverEntry final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  IntersectionObserverEntry(DOMHighResTimeStamp timestamp,
                            double intersection_ratio,
                            const FloatRect& bounding_client_rect,
                            const FloatRect* root_bounds,
                            const FloatRect& intersection_rect,
                            bool is_intersecting,
                            bool is_visible,
                            Element*);

  double time() const { return time_; }
  double intersectionRatio() const { return intersection_ratio_; }
  DOMRectReadOnly* boundingClientRect() const { return bounding_client_rect_; }
  DOMRectReadOnly* rootBounds() const { return root_bounds_; }
  DOMRectReadOnly* intersectionRect() const { return intersection_rect_; }
  bool isIntersecting() const { return is_intersecting_; }
  bool isVisible() const { return is_visible_; }
  Element* target() const { return target_.Get(); }

  void Trace(blink::Visitor*) override;

 private:
  DOMHighResTimeStamp time_;
  double intersection_ratio_;
  Member<DOMRectReadOnly> bounding_client_rect_;
  Member<DOMRectReadOnly> root_bounds_;
  Member<DOMRectReadOnly> intersection_rect_;
  Member<Element> target_;
  bool is_intersecting_;
  bool is_visible_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVER_ENTRY_H_
