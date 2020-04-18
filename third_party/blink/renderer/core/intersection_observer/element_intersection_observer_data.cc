// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/intersection_observer/element_intersection_observer_data.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observation.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observer.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observer_controller.h"

namespace blink {

ElementIntersectionObserverData::ElementIntersectionObserverData() = default;

IntersectionObservation* ElementIntersectionObserverData::GetObservationFor(
    IntersectionObserver& observer) {
  auto i = intersection_observations_.find(&observer);
  if (i == intersection_observations_.end())
    return nullptr;
  return i->value;
}

void ElementIntersectionObserverData::AddObserver(
    IntersectionObserver& observer) {
  intersection_observers_.insert(&observer);
}

void ElementIntersectionObserverData::RemoveObserver(
    IntersectionObserver& observer) {
  intersection_observers_.erase(&observer);
}

void ElementIntersectionObserverData::AddObservation(
    IntersectionObservation& observation) {
  DCHECK(observation.Observer());
  intersection_observations_.insert(observation.Observer(), &observation);
}

void ElementIntersectionObserverData::RemoveObservation(
    IntersectionObserver& observer) {
  intersection_observations_.erase(&observer);
}

void ElementIntersectionObserverData::ActivateValidIntersectionObservers(
    Node& node) {
  for (auto& observer : intersection_observers_) {
    Document* document = observer->TrackingDocument();
    if (!document)
      continue;
    document->EnsureIntersectionObserverController().AddTrackedObserver(
        *observer);
  }
  for (auto& observation : intersection_observations_)
    observation.value->UpdateShouldReportRootBoundsAfterDomChange();
}

void ElementIntersectionObserverData::DeactivateAllIntersectionObservers(
    Node& node) {
  node.GetDocument()
      .EnsureIntersectionObserverController()
      .RemoveTrackedObserversForRoot(node);
}

void ElementIntersectionObserverData::Trace(blink::Visitor* visitor) {
  visitor->Trace(intersection_observers_);
  visitor->Trace(intersection_observations_);
}

void ElementIntersectionObserverData::TraceWrappers(
    ScriptWrappableVisitor* visitor) const {
  for (auto& entry : intersection_observations_) {
    visitor->TraceWrappers(entry.key);
  }
}

}  // namespace blink
