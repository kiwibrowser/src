// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/intersection_observer/intersection_observer_controller.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"

namespace blink {

IntersectionObserverController* IntersectionObserverController::Create(
    Document* document) {
  IntersectionObserverController* result =
      new IntersectionObserverController(document);
  result->PauseIfNeeded();
  return result;
}

IntersectionObserverController::IntersectionObserverController(
    Document* document)
    : PausableObject(document), callback_fired_while_suspended_(false) {}

IntersectionObserverController::~IntersectionObserverController() = default;

void IntersectionObserverController::PostTaskToDeliverObservations() {
  DCHECK(GetExecutionContext());
  // TODO(ojan): These tasks decide whether to throttle a subframe, so they
  // need to be unthrottled, but we should throttle all the other tasks
  // (e.g. ones coming from the web page).
  GetExecutionContext()
      ->GetTaskRunner(TaskType::kInternalIntersectionObserver)
      ->PostTask(
          FROM_HERE,
          WTF::Bind(
              &IntersectionObserverController::DeliverIntersectionObservations,
              WrapWeakPersistent(this)));
}

void IntersectionObserverController::ScheduleIntersectionObserverForDelivery(
    IntersectionObserver& observer) {
  pending_intersection_observers_.insert(&observer);
  PostTaskToDeliverObservations();
}

void IntersectionObserverController::Unpause() {
  // If the callback fired while DOM objects were suspended, notifications might
  // be late, so deliver them right away (rather than waiting to fire again).
  if (callback_fired_while_suspended_) {
    callback_fired_while_suspended_ = false;
    PostTaskToDeliverObservations();
  }
}

void IntersectionObserverController::DeliverIntersectionObservations() {
  ExecutionContext* context = GetExecutionContext();
  if (!context) {
    pending_intersection_observers_.clear();
    return;
  }
  // TODO(yukishiino): Remove this CHECK once https://crbug.com/809784 gets
  // resolved.
  CHECK(!context->IsContextDestroyed());
  if (context->IsContextPaused()) {
    callback_fired_while_suspended_ = true;
    return;
  }
  pending_intersection_observers_.swap(intersection_observers_being_invoked_);
  for (auto& observer : intersection_observers_being_invoked_)
    observer->Deliver();
  intersection_observers_being_invoked_.clear();
}

void IntersectionObserverController::ComputeTrackedIntersectionObservations() {
  if (!GetExecutionContext())
    return;
  TRACE_EVENT0(
      "blink",
      "IntersectionObserverController::computeTrackedIntersectionObservations");
  for (auto& observer : tracked_intersection_observers_)
    observer->ComputeIntersectionObservations();
}

void IntersectionObserverController::AddTrackedObserver(
    IntersectionObserver& observer) {
  tracked_intersection_observers_.insert(&observer);
}

void IntersectionObserverController::RemoveTrackedObserversForRoot(
    const Node& root) {
  HeapVector<Member<IntersectionObserver>> to_remove;
  for (auto& observer : tracked_intersection_observers_) {
    if (observer->root() == &root)
      to_remove.push_back(observer);
  }
  tracked_intersection_observers_.RemoveAll(to_remove);
}

void IntersectionObserverController::Trace(blink::Visitor* visitor) {
  visitor->Trace(tracked_intersection_observers_);
  visitor->Trace(pending_intersection_observers_);
  visitor->Trace(intersection_observers_being_invoked_);
  PausableObject::Trace(visitor);
}

void IntersectionObserverController::TraceWrappers(
    ScriptWrappableVisitor* visitor) const {
  for (const auto& observer : pending_intersection_observers_)
    visitor->TraceWrappers(observer);
  for (const auto& observer : intersection_observers_being_invoked_)
    visitor->TraceWrappers(observer);
}

}  // namespace blink
