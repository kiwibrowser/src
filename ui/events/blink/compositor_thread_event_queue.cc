// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/blink/compositor_thread_event_queue.h"

#include "base/trace_event/trace_event.h"
#include "ui/events/blink/blink_event_util.h"
#include "ui/events/blink/web_input_event_traits.h"

namespace ui {

CompositorThreadEventQueue::CompositorThreadEventQueue() {}

CompositorThreadEventQueue::~CompositorThreadEventQueue() {}

void CompositorThreadEventQueue::Queue(
    std::unique_ptr<EventWithCallback> new_event,
    base::TimeTicks timestamp_now) {
  if (queue_.empty() ||
      !IsContinuousGestureEvent(new_event->event().GetType()) ||
      !IsCompatibleScrollorPinch(ToWebGestureEvent(new_event->event()),
                                 ToWebGestureEvent(queue_.back()->event()))) {
    if (new_event->first_original_event()) {
      // Trace could be nested as there might be multiple events in queue.
      // e.g. |ScrollUpdate|, |ScrollEnd|, and another scroll sequence.
      TRACE_EVENT_NESTABLE_ASYNC_BEGIN0("input",
                                        "CompositorThreadEventQueue::Queue",
                                        new_event->first_original_event());
    }
    queue_.emplace_back(std::move(new_event));
    return;
  }

  if (queue_.back()->CanCoalesceWith(*new_event)) {
    queue_.back()->CoalesceWith(new_event.get(), timestamp_now);
    return;
  }

  // Extract the last event in queue.
  std::unique_ptr<EventWithCallback> last_event = std::move(queue_.back());
  queue_.pop_back();
  DCHECK_LE(last_event->latency_info().trace_id(),
            new_event->latency_info().trace_id());
  LatencyInfo oldest_latency = last_event->latency_info();
  base::TimeTicks oldest_creation_timestamp = last_event->creation_timestamp();
  auto combined_original_events =
      std::make_unique<EventWithCallback::OriginalEventList>();
  combined_original_events->splice(combined_original_events->end(),
                                   last_event->original_events());
  combined_original_events->splice(combined_original_events->end(),
                                   new_event->original_events());

  // Extract the second last event in queue.
  std::unique_ptr<EventWithCallback> second_last_event = nullptr;
  if (!queue_.empty() &&
      IsCompatibleScrollorPinch(ToWebGestureEvent(new_event->event()),
                                ToWebGestureEvent(queue_.back()->event()))) {
    second_last_event = std::move(queue_.back());
    queue_.pop_back();
    DCHECK_LE(second_last_event->latency_info().trace_id(),
              oldest_latency.trace_id());
    oldest_latency = second_last_event->latency_info();
    oldest_creation_timestamp = second_last_event->creation_timestamp();
    combined_original_events->splice(combined_original_events->begin(),
                                     second_last_event->original_events());
  }

  std::pair<blink::WebGestureEvent, blink::WebGestureEvent> coalesced_events =
      CoalesceScrollAndPinch(
          second_last_event ? &ToWebGestureEvent(second_last_event->event())
                            : nullptr,
          ToWebGestureEvent(last_event->event()),
          ToWebGestureEvent(new_event->event()));

  std::unique_ptr<EventWithCallback> scroll_event =
      std::make_unique<EventWithCallback>(
          WebInputEventTraits::Clone(coalesced_events.first), oldest_latency,
          oldest_creation_timestamp, timestamp_now, nullptr);

  std::unique_ptr<EventWithCallback> pinch_event =
      std::make_unique<EventWithCallback>(
          WebInputEventTraits::Clone(coalesced_events.second), oldest_latency,
          oldest_creation_timestamp, timestamp_now,
          std::move(combined_original_events));

  queue_.emplace_back(std::move(scroll_event));
  queue_.emplace_back(std::move(pinch_event));
}

std::unique_ptr<EventWithCallback> CompositorThreadEventQueue::Pop() {
  std::unique_ptr<EventWithCallback> result;
  if (!queue_.empty()) {
    result = std::move(queue_.front());
    queue_.pop_front();
  }

  if (result->first_original_event()) {
    TRACE_EVENT_NESTABLE_ASYNC_END2(
        "input", "CompositorThreadEventQueue::Queue",
        result->first_original_event(), "type", result->event().GetType(),
        "coalesced_count", result->coalesced_count());
  }
  return result;
}

}  // namespace ui
