// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/blink/event_with_callback.h"

#include "base/time/time.h"
#include "ui/events/blink/blink_event_util.h"
#include "ui/events/blink/did_overscroll_params.h"
#include "ui/events/blink/web_input_event_traits.h"

using blink::WebInputEvent;
using blink::WebGestureEvent;

namespace ui {

EventWithCallback::EventWithCallback(
    WebScopedInputEvent event,
    const LatencyInfo& latency,
    base::TimeTicks timestamp_now,
    InputHandlerProxy::EventDispositionCallback callback)
    : event_(WebInputEventTraits::Clone(*event)),
      latency_(latency),
      creation_timestamp_(timestamp_now),
      last_coalesced_timestamp_(timestamp_now) {
  original_events_.emplace_back(std::move(event), latency, std::move(callback));
}

EventWithCallback::EventWithCallback(
    WebScopedInputEvent event,
    const LatencyInfo& latency,
    base::TimeTicks creation_timestamp,
    base::TimeTicks last_coalesced_timestamp,
    std::unique_ptr<OriginalEventList> original_events)
    : event_(std::move(event)),
      latency_(latency),
      creation_timestamp_(creation_timestamp),
      last_coalesced_timestamp_(last_coalesced_timestamp) {
  if (original_events)
    original_events_.splice(original_events_.end(), *original_events);
}

EventWithCallback::~EventWithCallback() {}

bool EventWithCallback::CanCoalesceWith(const EventWithCallback& other) const {
  return CanCoalesce(other.event(), event());
}

void EventWithCallback::CoalesceWith(EventWithCallback* other,
                                     base::TimeTicks timestamp_now) {
  // |other| should be a newer event than |this|.
  if (other->latency_.trace_id() >= 0 && latency_.trace_id() >= 0)
    DCHECK_GT(other->latency_.trace_id(), latency_.trace_id());

  // New events get coalesced into older events, and the newer timestamp
  // should always be preserved.
  const base::TimeTicks time_stamp = other->event().TimeStamp();
  Coalesce(other->event(), event_.get());
  event_->SetTimeStamp(time_stamp);

  // When coalescing two input events, we keep the oldest LatencyInfo
  // since it will represent the longest latency.
  other->latency_ = latency_;
  other->latency_.set_coalesced();

  // Move original events.
  original_events_.splice(original_events_.end(), other->original_events_);
  last_coalesced_timestamp_ = timestamp_now;
}

static bool HandledOnCompositorThread(
    InputHandlerProxy::EventDisposition disposition) {
  return (disposition != InputHandlerProxy::DID_NOT_HANDLE &&
          disposition !=
              InputHandlerProxy::DID_NOT_HANDLE_NON_BLOCKING_DUE_TO_FLING &&
          disposition != InputHandlerProxy::DID_HANDLE_NON_BLOCKING);
}

void EventWithCallback::RunCallbacks(
    InputHandlerProxy::EventDisposition disposition,
    const LatencyInfo& latency,
    std::unique_ptr<DidOverscrollParams> did_overscroll_params) {
  // |original_events_| could be empty if this is the scroll event extracted
  // from the matrix multiplication.
  if (original_events_.size() == 0)
    return;

  // Ack the oldest event with original latency.
  std::move(original_events_.front().callback_)
      .Run(disposition, std::move(original_events_.front().event_), latency,
           did_overscroll_params
               ? std::make_unique<DidOverscrollParams>(*did_overscroll_params)
               : nullptr);
  original_events_.pop_front();

  // If the event was handled on compositor thread, ack other events with
  // coalesced latency to avoid redundant tracking. If not, the event should
  // be handle on main thread, use the original latency instead.
  bool handled = HandledOnCompositorThread(disposition);
  for (auto& coalesced_event : original_events_) {
    if (handled) {
      coalesced_event.latency_ = latency;
      coalesced_event.latency_.set_coalesced();
    }
    std::move(coalesced_event.callback_)
        .Run(disposition, std::move(coalesced_event.event_),
             coalesced_event.latency_,
             did_overscroll_params
                 ? std::make_unique<DidOverscrollParams>(*did_overscroll_params)
                 : nullptr);
  }
}

EventWithCallback::OriginalEventWithCallback::OriginalEventWithCallback(
    WebScopedInputEvent event,
    const LatencyInfo& latency,
    InputHandlerProxy::EventDispositionCallback callback)
    : event_(std::move(event)),
      latency_(latency),
      callback_(std::move(callback)) {}

EventWithCallback::OriginalEventWithCallback::~OriginalEventWithCallback() {}

}  // namespace ui
