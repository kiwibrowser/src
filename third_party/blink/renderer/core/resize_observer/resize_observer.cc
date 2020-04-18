// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/resize_observer/resize_observer.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_resize_observer_callback.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/layout/adjust_for_absolute_zoom.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/resize_observer/resize_observation.h"
#include "third_party/blink/renderer/core/resize_observer/resize_observer_controller.h"
#include "third_party/blink/renderer/core/resize_observer/resize_observer_entry.h"

namespace blink {

ResizeObserver* ResizeObserver::Create(Document& document,
                                       V8ResizeObserverCallback* callback) {
  return new ResizeObserver(callback, document);
}

ResizeObserver* ResizeObserver::Create(Document& document, Delegate* delegate) {
  return new ResizeObserver(delegate, document);
}

ResizeObserver::ResizeObserver(V8ResizeObserverCallback* callback,
                               Document& document)
    : ContextClient(&document),
      callback_(callback),
      skipped_observations_(false),
      element_size_changed_(false) {
  DCHECK(callback_);
  controller_ = &document.EnsureResizeObserverController();
  controller_->AddObserver(*this);
}

ResizeObserver::ResizeObserver(Delegate* delegate, Document& document)
    : ContextClient(&document),
      delegate_(delegate),
      skipped_observations_(false),
      element_size_changed_(false) {
  DCHECK(delegate_);
  controller_ = &document.EnsureResizeObserverController();
  controller_->AddObserver(*this);
}

void ResizeObserver::observe(Element* target) {
  auto& observer_map = target->EnsureResizeObserverData();
  if (observer_map.Contains(this))
    return;  // Already registered.

  auto* observation = new ResizeObservation(target, this);
  observations_.insert(observation);
  observer_map.Set(this, observation);

  if (LocalFrameView* frame_view = target->GetDocument().View())
    frame_view->ScheduleAnimation();
}

void ResizeObserver::unobserve(Element* target) {
  auto* observer_map = target ? target->ResizeObserverData() : nullptr;
  if (!observer_map)
    return;
  auto observation = observer_map->find(this);
  if (observation != observer_map->end()) {
    observations_.erase((*observation).value);
    auto index = active_observations_.Find((*observation).value);
    if (index != kNotFound) {
      active_observations_.EraseAt(index);
    }
    observer_map->erase(observation);
  }
}

void ResizeObserver::disconnect() {
  ObservationList observations;
  observations_.Swap(observations);

  for (auto& observation : observations) {
    Element* target = (*observation).Target();
    if (target)
      target->EnsureResizeObserverData().erase(this);
  }
  ClearObservations();
}

size_t ResizeObserver::GatherObservations(size_t deeper_than) {
  DCHECK(active_observations_.IsEmpty());

  size_t min_observed_depth = ResizeObserverController::kDepthBottom;
  if (!element_size_changed_)
    return min_observed_depth;
  for (auto& observation : observations_) {
    if (!observation->ObservationSizeOutOfSync())
      continue;
    auto depth = observation->TargetDepth();
    if (depth > deeper_than) {
      active_observations_.push_back(*observation);
      min_observed_depth = std::min(min_observed_depth, depth);
    } else {
      skipped_observations_ = true;
    }
  }
  return min_observed_depth;
}

void ResizeObserver::DeliverObservations() {
  // We can only clear this flag after all observations have been
  // broadcast.
  element_size_changed_ = skipped_observations_;
  if (active_observations_.IsEmpty())
    return;

  HeapVector<Member<ResizeObserverEntry>> entries;

  for (auto& observation : active_observations_) {
    // In case that the observer and the target belong to different execution
    // contexts and the target's execution context is already gone, then skip
    // such a target.
    ExecutionContext* execution_context =
        observation->Target()->GetExecutionContext();
    if (!execution_context || execution_context->IsContextDestroyed())
      continue;

    LayoutPoint location = observation->ComputeTargetLocation();
    LayoutSize size = observation->ComputeTargetSize();
    observation->SetObservationSize(size);

    LayoutRect content_rect(location, size);
    if (observation->Target()->GetLayoutObject()) {
      // Must adjust for zoom in order to report non-zoomed size.
      const ComputedStyle& style =
          observation->Target()->GetLayoutObject()->StyleRef();
      content_rect.SetX(
          AdjustForAbsoluteZoom::AdjustLayoutUnit(content_rect.X(), style));
      content_rect.SetY(
          AdjustForAbsoluteZoom::AdjustLayoutUnit(content_rect.Y(), style));
      content_rect.SetWidth(
          AdjustForAbsoluteZoom::AdjustLayoutUnit(content_rect.Width(), style));
      content_rect.SetHeight(AdjustForAbsoluteZoom::AdjustLayoutUnit(
          content_rect.Height(), style));
    }
    auto* entry = new ResizeObserverEntry(observation->Target(), content_rect);
    entries.push_back(entry);
  }

  if (entries.size() == 0) {
    // No entry to report.
    // Note that, if |active_observations_| is not empty but |entries| is empty,
    // it means that it's possible that no target element is making |callback_|
    // alive. In this case, we must not touch |callback_|.
    ClearObservations();
    return;
  }

  DCHECK(callback_ || delegate_);
  if (callback_)
    callback_->InvokeAndReportException(this, entries, this);
  if (delegate_)
    delegate_->OnResize(entries);
  ClearObservations();
}

void ResizeObserver::ClearObservations() {
  active_observations_.clear();
  skipped_observations_ = false;
}

void ResizeObserver::ElementSizeChanged() {
  element_size_changed_ = true;
  if (controller_)
    controller_->ObserverChanged();
}

bool ResizeObserver::HasPendingActivity() const {
  return !observations_.IsEmpty();
}

void ResizeObserver::Trace(blink::Visitor* visitor) {
  visitor->Trace(callback_);
  visitor->Trace(delegate_);
  visitor->Trace(observations_);
  visitor->Trace(active_observations_);
  visitor->Trace(controller_);
  ScriptWrappable::Trace(visitor);
  ContextClient::Trace(visitor);
}

void ResizeObserver::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(callback_);
  ScriptWrappable::TraceWrappers(visitor);
}

}  // namespace blink
