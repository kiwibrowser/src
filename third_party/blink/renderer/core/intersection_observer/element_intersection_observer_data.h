// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_INTERSECTION_OBSERVER_ELEMENT_INTERSECTION_OBSERVER_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_INTERSECTION_OBSERVER_ELEMENT_INTERSECTION_OBSERVER_DATA_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class Node;
class IntersectionObservation;
class IntersectionObserver;

class ElementIntersectionObserverData
    : public GarbageCollected<ElementIntersectionObserverData>,
      public TraceWrapperBase {
 public:
  ElementIntersectionObserverData();

  IntersectionObservation* GetObservationFor(IntersectionObserver&);
  void AddObserver(IntersectionObserver&);
  void RemoveObserver(IntersectionObserver&);
  void AddObservation(IntersectionObservation&);
  void RemoveObservation(IntersectionObserver&);
  void ActivateValidIntersectionObservers(Node&);
  void DeactivateAllIntersectionObservers(Node&);

  void Trace(blink::Visitor*);
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  const char* NameInHeapSnapshot() const override {
    return "ElementIntersectionObserverData";
  }

 private:
  // IntersectionObservers for which the Node owning this data is root.
  HeapHashSet<WeakMember<IntersectionObserver>> intersection_observers_;
  // IntersectionObservations for which the Node owning this data is target.
  HeapHashMap<TraceWrapperMember<IntersectionObserver>,
              Member<IntersectionObservation>>
      intersection_observations_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_INTERSECTION_OBSERVER_ELEMENT_INTERSECTION_OBSERVER_DATA_H_
