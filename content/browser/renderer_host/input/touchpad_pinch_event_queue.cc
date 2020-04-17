// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/touchpad_pinch_event_queue.h"

#include "base/trace_event/trace_event.h"
#include "third_party/blink/public/platform/web_mouse_wheel_event.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/blink/blink_event_util.h"
#include "ui/latency/latency_info.h"

namespace content {

namespace {

blink::WebMouseWheelEvent CreateSyntheticWheelFromTouchpadPinchEvent(
    const blink::WebGestureEvent& pinch_event) {
  DCHECK_EQ(blink::WebInputEvent::kGesturePinchUpdate, pinch_event.GetType());

  // For pinch gesture events, similar to ctrl+wheel zooming, allow content to
  // prevent the browser from zooming by sending fake wheel events with the ctrl
  // modifier set when we see trackpad pinch gestures.  Ideally we'd someday get
  // a platform 'pinch' event and send that instead.
  blink::WebMouseWheelEvent wheel_event(
      blink::WebInputEvent::kMouseWheel,
      pinch_event.GetModifiers() | blink::WebInputEvent::kControlKey,
      pinch_event.TimeStamp());
  wheel_event.SetPositionInWidget(pinch_event.PositionInWidget());
  wheel_event.SetPositionInScreen(pinch_event.PositionInScreen());
  wheel_event.delta_x = 0;

  // The function to convert scales to deltaY values is designed to be
  // compatible with websites existing use of wheel events, and with existing
  // Windows trackpad behavior.  In particular, we want:
  //  - deltas should accumulate via addition: f(s1*s2)==f(s1)+f(s2)
  //  - deltas should invert via negation: f(1/s) == -f(s)
  //  - zoom in should be positive: f(s) > 0 iff s > 1
  //  - magnitude roughly matches wheels: f(2) > 25 && f(2) < 100
  //  - a formula that's relatively easy to use from JavaScript
  // Note that 'wheel' event deltaY values have their sign inverted.  So to
  // convert a wheel deltaY back to a scale use Math.exp(-deltaY/100).
  DCHECK_GT(pinch_event.data.pinch_update.scale, 0);
  wheel_event.delta_y = 100.0f * log(pinch_event.data.pinch_update.scale);
  wheel_event.has_precise_scrolling_deltas = true;
  wheel_event.wheel_ticks_x = 0;
  wheel_event.wheel_ticks_y = pinch_event.data.pinch_update.scale > 1 ? 1 : -1;

  return wheel_event;
}

}  // namespace

// This is a single queued pinch event to which we add trace events.
class QueuedTouchpadPinchEvent : public GestureEventWithLatencyInfo {
 public:
  QueuedTouchpadPinchEvent(const GestureEventWithLatencyInfo& original_event)
      : GestureEventWithLatencyInfo(original_event) {
    TRACE_EVENT_ASYNC_BEGIN0("input", "TouchpadPinchEventQueue::QueueEvent",
                             this);
  }

  ~QueuedTouchpadPinchEvent() {
    TRACE_EVENT_ASYNC_END0("input", "TouchpadPinchEventQueue::QueueEvent",
                           this);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(QueuedTouchpadPinchEvent);
};

TouchpadPinchEventQueue::TouchpadPinchEventQueue(
    TouchpadPinchEventQueueClient* client)
    : client_(client) {
  DCHECK(client_);
}

TouchpadPinchEventQueue::~TouchpadPinchEventQueue() = default;

void TouchpadPinchEventQueue::QueueEvent(
    const GestureEventWithLatencyInfo& event) {
  TRACE_EVENT0("input", "TouchpadPinchEventQueue::QueueEvent");

  if (!pinch_queue_.empty()) {
    QueuedTouchpadPinchEvent* last_event = pinch_queue_.back().get();
    if (last_event->CanCoalesceWith(event)) {
      // Terminate the LatencyInfo of the event before it gets coalesced away.
      event.latency.AddLatencyNumber(
          ui::INPUT_EVENT_LATENCY_TERMINATED_NO_SWAP_COMPONENT, 0);

      last_event->CoalesceWith(event);
      DCHECK_EQ(blink::WebInputEvent::kGesturePinchUpdate,
                last_event->event.GetType());
      TRACE_EVENT_INSTANT1("input",
                           "TouchpadPinchEventQueue::CoalescedPinchEvent",
                           TRACE_EVENT_SCOPE_THREAD, "scale",
                           last_event->event.data.pinch_update.scale);
      return;
    }
  }

  pinch_queue_.push_back(std::make_unique<QueuedTouchpadPinchEvent>(event));
  TryForwardNextEventToRenderer();
}

void TouchpadPinchEventQueue::ProcessMouseWheelAck(
    InputEventAckSource ack_source,
    InputEventAckState ack_result,
    const ui::LatencyInfo& latency_info) {
  TRACE_EVENT0("input", "TouchpadPinchEventQueue::ProcessMouseWheelAck");
  if (!pinch_event_awaiting_ack_)
    return;

  pinch_event_awaiting_ack_->latency.AddNewLatencyFrom(latency_info);
  client_->OnGestureEventForPinchAck(*pinch_event_awaiting_ack_, ack_source,
                                     ack_result);

  pinch_event_awaiting_ack_.reset();
  TryForwardNextEventToRenderer();
}

void TouchpadPinchEventQueue::TryForwardNextEventToRenderer() {
  TRACE_EVENT0("input",
               "TouchpadPinchEventQueue::TryForwardNextEventToRenderer");

  if (pinch_queue_.empty() || pinch_event_awaiting_ack_)
    return;

  pinch_event_awaiting_ack_ = std::move(pinch_queue_.front());
  pinch_queue_.pop_front();

  if (pinch_event_awaiting_ack_->event.GetType() ==
          blink::WebInputEvent::kGesturePinchBegin ||
      pinch_event_awaiting_ack_->event.GetType() ==
          blink::WebInputEvent::kGesturePinchEnd) {
    // Wheel event listeners are given individual events with no phase
    // information, so we don't need to do anything at the beginning or
    // end of a pinch.
    // TODO(565980): Consider sending the rest of the wheel events as
    // non-blocking if the first wheel event of the pinch sequence is not
    // consumed.
    client_->OnGestureEventForPinchAck(*pinch_event_awaiting_ack_,
                                       InputEventAckSource::BROWSER,
                                       INPUT_EVENT_ACK_STATE_IGNORED);
    pinch_event_awaiting_ack_.reset();
    TryForwardNextEventToRenderer();
    return;
  }

  const MouseWheelEventWithLatencyInfo synthetic_wheel(
      CreateSyntheticWheelFromTouchpadPinchEvent(
          pinch_event_awaiting_ack_->event),
      pinch_event_awaiting_ack_->latency);

  client_->SendMouseWheelEventForPinchImmediately(synthetic_wheel);
}

bool TouchpadPinchEventQueue::has_pending() const {
  return !pinch_queue_.empty() || pinch_event_awaiting_ack_;
}

}  // namespace content
