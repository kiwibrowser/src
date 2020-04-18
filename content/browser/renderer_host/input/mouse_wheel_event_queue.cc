// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/mouse_wheel_event_queue.h"

#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "content/common/input/input_event_dispatch_type.h"
#include "content/public/common/content_features.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/blink/web_input_event_traits.h"

using blink::WebGestureEvent;
using blink::WebInputEvent;
using blink::WebMouseWheelEvent;
using ui::LatencyInfo;

namespace content {

// This class represents a single queued mouse wheel event. Its main use
// is that it is reported via trace events.
class QueuedWebMouseWheelEvent : public MouseWheelEventWithLatencyInfo {
 public:
  QueuedWebMouseWheelEvent(const MouseWheelEventWithLatencyInfo& original_event)
      : MouseWheelEventWithLatencyInfo(original_event) {
    TRACE_EVENT_ASYNC_BEGIN0("input", "MouseWheelEventQueue::QueueEvent", this);
  }

  ~QueuedWebMouseWheelEvent() {
    TRACE_EVENT_ASYNC_END0("input", "MouseWheelEventQueue::QueueEvent", this);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(QueuedWebMouseWheelEvent);
};

MouseWheelEventQueue::MouseWheelEventQueue(MouseWheelEventQueueClient* client,
                                           bool enable_scroll_latching)
    : client_(client),
      needs_scroll_begin_when_scroll_latching_disabled_(true),
      needs_scroll_end_when_scroll_latching_disabled_(false),
      enable_scroll_latching_(enable_scroll_latching),
      enable_async_wheel_events_(
          base::FeatureList::IsEnabled(features::kAsyncWheelEvents) &&
          enable_scroll_latching),
      send_wheel_events_async_(false),
      scrolling_device_(blink::kWebGestureDeviceUninitialized) {
  DCHECK(client);
}

MouseWheelEventQueue::~MouseWheelEventQueue() {
}

void MouseWheelEventQueue::QueueEvent(
    const MouseWheelEventWithLatencyInfo& event) {
  TRACE_EVENT0("input", "MouseWheelEventQueue::QueueEvent");

  if (event_sent_for_gesture_ack_ && !wheel_queue_.empty()) {
    QueuedWebMouseWheelEvent* last_event = wheel_queue_.back().get();
    if (last_event->CanCoalesceWith(event)) {
      last_event->CoalesceWith(event);
      TRACE_EVENT_INSTANT2("input", "MouseWheelEventQueue::CoalescedWheelEvent",
                           TRACE_EVENT_SCOPE_THREAD, "total_dx",
                           last_event->event.delta_x, "total_dy",
                           last_event->event.delta_y);
      return;
    }
  }

  wheel_queue_.push_back(std::make_unique<QueuedWebMouseWheelEvent>(event));
  TryForwardNextEventToRenderer();
  LOCAL_HISTOGRAM_COUNTS_100("Renderer.WheelQueueSize", wheel_queue_.size());
}

bool MouseWheelEventQueue::CanGenerateGestureScroll(
    InputEventAckState ack_result) const {
  if (ack_result == INPUT_EVENT_ACK_STATE_CONSUMED) {
    TRACE_EVENT_INSTANT0("input", "Wheel Event Consumed",
                         TRACE_EVENT_SCOPE_THREAD);
    return false;
  }

  if (!ui::WebInputEventTraits::CanCauseScroll(
          event_sent_for_gesture_ack_->event)) {
    TRACE_EVENT_INSTANT0("input", "Wheel Event Cannot Cause Scroll",
                         TRACE_EVENT_SCOPE_THREAD);
    return false;
  }

  if (event_sent_for_gesture_ack_->event.resending_plugin_id != -1) {
    TRACE_EVENT_INSTANT0("input", "Wheel Event Resending Plugin Id Is Not -1",
                         TRACE_EVENT_SCOPE_THREAD);
    return false;
  }

  if (scrolling_device_ != blink::kWebGestureDeviceUninitialized &&
      scrolling_device_ != blink::kWebGestureDeviceTouchpad) {
    TRACE_EVENT_INSTANT0("input",
                         "Autoscroll or Touchscreen Scroll In Progress",
                         TRACE_EVENT_SCOPE_THREAD);
    return false;
  }

  return true;
}

void MouseWheelEventQueue::ProcessMouseWheelAck(
    InputEventAckSource ack_source,
    InputEventAckState ack_result,
    const LatencyInfo& latency_info) {
  TRACE_EVENT0("input", "MouseWheelEventQueue::ProcessMouseWheelAck");
  if (!event_sent_for_gesture_ack_)
    return;

  event_sent_for_gesture_ack_->latency.AddNewLatencyFrom(latency_info);
  client_->OnMouseWheelEventAck(*event_sent_for_gesture_ack_, ack_source,
                                ack_result);

  // If event wasn't consumed then generate a gesture scroll for it.
  if (CanGenerateGestureScroll(ack_result)) {
    WebGestureEvent scroll_update(
        WebInputEvent::kGestureScrollUpdate, WebInputEvent::kNoModifiers,
        event_sent_for_gesture_ack_->event.TimeStamp(),
        blink::kWebGestureDeviceTouchpad);

    scroll_update.SetPositionInWidget(
        event_sent_for_gesture_ack_->event.PositionInWidget());
    scroll_update.SetPositionInScreen(
        event_sent_for_gesture_ack_->event.PositionInScreen());
    scroll_update.resending_plugin_id = -1;

    // Swap X & Y if Shift is down and when there is no horizontal movement.
    if ((event_sent_for_gesture_ack_->event.GetModifiers() &
         WebInputEvent::kShiftKey) != 0 &&
        event_sent_for_gesture_ack_->event.delta_x == 0) {
      scroll_update.data.scroll_update.delta_x =
          event_sent_for_gesture_ack_->event.delta_y;
      scroll_update.data.scroll_update.delta_y =
          event_sent_for_gesture_ack_->event.delta_x;
    } else {
      scroll_update.data.scroll_update.delta_x =
          event_sent_for_gesture_ack_->event.delta_x;
      scroll_update.data.scroll_update.delta_y =
          event_sent_for_gesture_ack_->event.delta_y;
    }
    // Only OSX populates the phase and momentumPhase; so
    // |inertialPhase| will be UnknownMomentumPhase on all other platforms.
    if (event_sent_for_gesture_ack_->event.momentum_phase !=
        blink::WebMouseWheelEvent::kPhaseNone) {
      scroll_update.data.scroll_update.inertial_phase =
          WebGestureEvent::kMomentumPhase;
    } else if (event_sent_for_gesture_ack_->event.phase !=
               blink::WebMouseWheelEvent::kPhaseNone) {
      scroll_update.data.scroll_update.inertial_phase =
          WebGestureEvent::kNonMomentumPhase;
    }
    if (event_sent_for_gesture_ack_->event.scroll_by_page) {
      scroll_update.data.scroll_update.delta_units = WebGestureEvent::kPage;

      // Turn page scrolls into a *single* page scroll because
      // the magnitude the number of ticks is lost when coalescing.
      if (scroll_update.data.scroll_update.delta_x)
        scroll_update.data.scroll_update.delta_x =
            scroll_update.data.scroll_update.delta_x > 0 ? 1 : -1;
      if (scroll_update.data.scroll_update.delta_y)
        scroll_update.data.scroll_update.delta_y =
            scroll_update.data.scroll_update.delta_y > 0 ? 1 : -1;
    } else {
      scroll_update.data.scroll_update.delta_units =
          event_sent_for_gesture_ack_->event.has_precise_scrolling_deltas
              ? WebGestureEvent::kPrecisePixels
              : WebGestureEvent::kPixels;

      if (event_sent_for_gesture_ack_->event.rails_mode ==
          WebInputEvent::kRailsModeVertical)
        scroll_update.data.scroll_update.delta_x = 0;
      if (event_sent_for_gesture_ack_->event.rails_mode ==
          WebInputEvent::kRailsModeHorizontal)
        scroll_update.data.scroll_update.delta_y = 0;
    }

    bool current_phase_ended = false;
    bool scroll_phase_ended = false;
    bool momentum_phase_ended = false;
    bool has_phase_info = false;

    if (event_sent_for_gesture_ack_->event.phase !=
            blink::WebMouseWheelEvent::kPhaseNone ||
        event_sent_for_gesture_ack_->event.momentum_phase !=
            blink::WebMouseWheelEvent::kPhaseNone) {
      has_phase_info = true;
      scroll_phase_ended = event_sent_for_gesture_ack_->event.phase ==
                               blink::WebMouseWheelEvent::kPhaseEnded ||
                           event_sent_for_gesture_ack_->event.phase ==
                               blink::WebMouseWheelEvent::kPhaseCancelled;
      momentum_phase_ended =
          event_sent_for_gesture_ack_->event.momentum_phase ==
              blink::WebMouseWheelEvent::kPhaseEnded ||
          event_sent_for_gesture_ack_->event.momentum_phase ==
              blink::WebMouseWheelEvent::kPhaseCancelled;
      current_phase_ended = scroll_phase_ended || momentum_phase_ended;
    }

    bool needs_update = scroll_update.data.scroll_update.delta_x != 0 ||
                        scroll_update.data.scroll_update.delta_y != 0;

    // For every GSU event record whether it is latched or not.
    if (needs_update)
      RecordLatchingUmaMetric(client_->IsWheelScrollInProgress());

    if (enable_scroll_latching_) {
      bool synthetic = event_sent_for_gesture_ack_->event.has_synthetic_phase;
      if (event_sent_for_gesture_ack_->event.phase ==
          blink::WebMouseWheelEvent::kPhaseBegan) {
        // Wheel event with phaseBegan must have non-zero deltas.
        DCHECK(needs_update);
        send_wheel_events_async_ = true;

        if (!client_->IsWheelScrollInProgress())
          SendScrollBegin(scroll_update, synthetic);
      }

      if (needs_update) {
        // It is possible that the wheel event with phaseBegan is consumed and
        // no GSB is sent.
        if (!client_->IsWheelScrollInProgress())
          SendScrollBegin(scroll_update, synthetic);
        ui::LatencyInfo latency = ui::LatencyInfo(ui::SourceEventType::WHEEL);
        latency.AddLatencyNumber(
            ui::INPUT_EVENT_LATENCY_GENERATE_SCROLL_UPDATE_FROM_MOUSE_WHEEL, 0);
        client_->ForwardGestureEventWithLatencyInfo(scroll_update, latency);
      }

      if (current_phase_ended && client_->IsWheelScrollInProgress()) {
        // Send GSE when scroll latching is enabled, GSB is sent, and no fling
        // is going to happen next.
        SendScrollEnd(scroll_update, synthetic);
      }
    } else {  // !enable_scroll_latching_

      // If there is no update to send and the current phase is ended yet a GSB
      // needs to be sent, this event sequence doesn't need to be generated
      // because the events generated will be a GSB (non-synthetic) and GSE
      // (non-synthetic). This situation arises when OSX generates double
      // phase end information.
      bool empty_sequence =
          !needs_update && needs_scroll_begin_when_scroll_latching_disabled_ &&
          current_phase_ended &&
          !event_sent_for_gesture_ack_->event.has_synthetic_phase;
      if (needs_update || !empty_sequence) {
        if (needs_scroll_begin_when_scroll_latching_disabled_) {
          // If no GSB has been sent, it will be a non-synthetic GSB.
          SendScrollBegin(scroll_update, false);
        } else if (has_phase_info) {
          // If a GSB has been sent, generate a synthetic GSB if we have phase
          // information. This should be removed once crbug.com/526463 is
          // fully implemented.
          SendScrollBegin(scroll_update, true);
        }

        if (needs_update) {
          ui::LatencyInfo latency = ui::LatencyInfo(ui::SourceEventType::WHEEL);
          latency.AddLatencyNumber(
              ui::INPUT_EVENT_LATENCY_GENERATE_SCROLL_UPDATE_FROM_MOUSE_WHEEL,
              0);
          client_->ForwardGestureEventWithLatencyInfo(scroll_update, latency);
        }

        if (current_phase_ended) {
          // Non-synthetic GSEs are sent when the current phase is canceled or
          // ended.
          SendScrollEnd(scroll_update, false);
        } else if (has_phase_info) {
          // Generate a synthetic GSE for every update to force hit testing so
          // that the non-latching behavior is preserved. Remove once
          // crbug.com/526463 is fully implemented.
          SendScrollEnd(scroll_update, true);
        } else if (!has_phase_info) {
          SendScrollEnd(scroll_update, false);
        }
      }
    }
  }

  event_sent_for_gesture_ack_.reset();
  TryForwardNextEventToRenderer();
}

void MouseWheelEventQueue::OnGestureScrollEvent(
    const GestureEventWithLatencyInfo& gesture_event) {
  if (gesture_event.event.GetType() ==
      blink::WebInputEvent::kGestureScrollBegin) {
    scrolling_device_ = gesture_event.event.SourceDevice();
  } else if (scrolling_device_ == gesture_event.event.SourceDevice() &&
             gesture_event.event.GetType() ==
                 blink::WebInputEvent::kGestureScrollEnd) {
    scrolling_device_ = blink::kWebGestureDeviceUninitialized;
  } else if (gesture_event.event.GetType() ==
             blink::WebInputEvent::kGestureFlingStart) {
    // With browser side fling we shouldn't reset scrolling_device_ on GFS since
    // the fling_controller processes the GFS to generate and send GSU events.
  }
}

void MouseWheelEventQueue::TryForwardNextEventToRenderer() {
  TRACE_EVENT0("input", "MouseWheelEventQueue::TryForwardNextEventToRenderer");

  if (wheel_queue_.empty() || event_sent_for_gesture_ack_)
    return;

  event_sent_for_gesture_ack_ = std::move(wheel_queue_.front());
  wheel_queue_.pop_front();

  if (enable_async_wheel_events_) {
    DCHECK(event_sent_for_gesture_ack_->event.phase !=
               blink::WebMouseWheelEvent::kPhaseNone ||
           event_sent_for_gesture_ack_->event.momentum_phase !=
               blink::WebMouseWheelEvent::kPhaseNone);
    if (event_sent_for_gesture_ack_->event.phase ==
        blink::WebMouseWheelEvent::kPhaseBegan) {
      send_wheel_events_async_ = false;
    } else if (send_wheel_events_async_) {
      event_sent_for_gesture_ack_->event.dispatch_type =
          WebInputEvent::kEventNonBlocking;
    }
  }

  client_->SendMouseWheelEventImmediately(*event_sent_for_gesture_ack_);
}

void MouseWheelEventQueue::SendScrollEnd(WebGestureEvent update_event,
                                         bool synthetic) {
  DCHECK(enable_scroll_latching_ ||
         (synthetic && !needs_scroll_end_when_scroll_latching_disabled_) ||
         needs_scroll_end_when_scroll_latching_disabled_);

  DCHECK(client_->IsWheelScrollInProgress());

  WebGestureEvent scroll_end(update_event);
  scroll_end.SetTimeStamp(ui::EventTimeForNow());
  scroll_end.SetType(WebInputEvent::kGestureScrollEnd);
  scroll_end.resending_plugin_id = -1;
  scroll_end.data.scroll_end.synthetic = synthetic;
  scroll_end.data.scroll_end.inertial_phase =
      update_event.data.scroll_update.inertial_phase;
  scroll_end.data.scroll_end.delta_units =
      update_event.data.scroll_update.delta_units;

  if (!synthetic) {
    needs_scroll_begin_when_scroll_latching_disabled_ = true;
    needs_scroll_end_when_scroll_latching_disabled_ = false;
  }
  client_->ForwardGestureEventWithLatencyInfo(
      scroll_end, ui::LatencyInfo(ui::SourceEventType::WHEEL));
}

void MouseWheelEventQueue::SendScrollBegin(
    const WebGestureEvent& gesture_update,
    bool synthetic) {
  DCHECK(enable_scroll_latching_ ||
         (synthetic && !needs_scroll_begin_when_scroll_latching_disabled_) ||
         needs_scroll_begin_when_scroll_latching_disabled_);

  DCHECK(!client_->IsWheelScrollInProgress());

  WebGestureEvent scroll_begin(gesture_update);
  scroll_begin.SetType(WebInputEvent::kGestureScrollBegin);
  scroll_begin.data.scroll_begin.synthetic = synthetic;
  scroll_begin.data.scroll_begin.inertial_phase =
      gesture_update.data.scroll_update.inertial_phase;
  scroll_begin.data.scroll_begin.delta_x_hint =
      gesture_update.data.scroll_update.delta_x;
  scroll_begin.data.scroll_begin.delta_y_hint =
      gesture_update.data.scroll_update.delta_y;
  scroll_begin.data.scroll_begin.target_viewport = false;
  scroll_begin.data.scroll_begin.delta_hint_units =
      gesture_update.data.scroll_update.delta_units;

  needs_scroll_begin_when_scroll_latching_disabled_ = false;
  needs_scroll_end_when_scroll_latching_disabled_ = true;
  client_->ForwardGestureEventWithLatencyInfo(
      scroll_begin, ui::LatencyInfo(ui::SourceEventType::WHEEL));
}

void MouseWheelEventQueue::RecordLatchingUmaMetric(bool latched) {
  UMA_HISTOGRAM_BOOLEAN("WheelScrolling.WasLatched", latched);
}

}  // namespace content
