// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVATION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVATION_H_

#include "third_party/blink/renderer/core/dom/dom_high_res_time_stamp.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class Element;
class IntersectionObserver;

// IntersectionObservation represents the result of calling
// IntersectionObserver::observe(target) for some target element; it tracks the
// intersection between a single target element and the IntersectionObserver's
// root.  It is an implementation-internal class without any exposed interface.
class IntersectionObservation final
    : public GarbageCollected<IntersectionObservation> {
 public:
  IntersectionObservation(IntersectionObserver&, Element&);

  IntersectionObserver* Observer() const { return observer_.Get(); }
  Element* Target() const { return target_; }
  unsigned LastThresholdIndex() const { return last_threshold_index_; }
  void ComputeIntersectionObservations(DOMHighResTimeStamp);
  void Disconnect();
  void UpdateShouldReportRootBoundsAfterDomChange();

  void Trace(blink::Visitor*);

 private:
  void SetLastThresholdIndex(unsigned index) { last_threshold_index_ = index; }
  void SetWasVisible(bool last_is_visible) {
    last_is_visible_ = last_is_visible ? 1 : 0;
  }

  Member<IntersectionObserver> observer_;
  WeakMember<Element> target_;

  unsigned should_report_root_bounds_ : 1;
  unsigned last_is_visible_ : 1;
  unsigned last_threshold_index_ : 29;
  static const unsigned kMaxThresholdIndex = (unsigned)0x20000000;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVATION_H_
