// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/render_widget_host_latency_tracker.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "build/build_config.h"
#include "components/rappor/public/rappor_utils.h"
#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "ui/events/blink/web_input_event_traits.h"
#include "ui/latency/latency_histogram_macros.h"

using blink::WebGestureEvent;
using blink::WebInputEvent;
using blink::WebMouseEvent;
using blink::WebMouseWheelEvent;
using blink::WebTouchEvent;
using ui::LatencyInfo;

namespace content {
namespace {

ukm::SourceId GenerateUkmSourceId() {
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  return ukm_recorder ? ukm_recorder->GetNewSourceID() : ukm::kInvalidSourceId;
}

}  // namespace

RenderWidgetHostLatencyTracker::RenderWidgetHostLatencyTracker(
    RenderWidgetHostDelegate* delegate)
    : ukm_source_id_(GenerateUkmSourceId()),
      last_event_id_(0),
      latency_component_id_(0),
      has_seen_first_gesture_scroll_update_(false),
      active_multi_finger_gesture_(false),
      touch_start_default_prevented_(false),
      render_widget_host_delegate_(delegate) {}

RenderWidgetHostLatencyTracker::~RenderWidgetHostLatencyTracker() {}

void RenderWidgetHostLatencyTracker::Initialize(int routing_id,
                                                int process_id) {
  DCHECK_EQ(0, last_event_id_);
  DCHECK_EQ(0, latency_component_id_);
  last_event_id_ = static_cast<int64_t>(process_id) << 32;
  latency_component_id_ = routing_id | last_event_id_;
}

void RenderWidgetHostLatencyTracker::ComputeInputLatencyHistograms(
    WebInputEvent::Type type,
    int64_t latency_component_id,
    const LatencyInfo& latency,
    InputEventAckState ack_result) {
  // If this event was coalesced into another event, ignore it, as the event it
  // was coalesced into will reflect the full latency.
  if (latency.coalesced())
    return;

  if (latency.source_event_type() == ui::SourceEventType::UNKNOWN ||
      latency.source_event_type() == ui::SourceEventType::OTHER) {
    return;
  }

  // The event will have gone through OnInputEvent(). So the BEGIN_RWH component
  // should always be available here.
  LatencyInfo::LatencyComponent rwh_component;
  bool found_component =
      latency.FindLatency(ui::INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT,
                          latency_component_id, &rwh_component);
  DCHECK(found_component);
  DCHECK_EQ(rwh_component.event_count, 1u);

  bool multi_finger_touch_gesture =
      WebInputEvent::IsTouchEventType(type) && active_multi_finger_gesture_;

  bool action_prevented = ack_result == INPUT_EVENT_ACK_STATE_CONSUMED;
  // Touchscreen tap and scroll gestures depend on the disposition of the touch
  // start and the current touch. For touch start,
  // touch_start_default_prevented_ == (ack_result ==
  // INPUT_EVENT_ACK_STATE_CONSUMED).
  if (WebInputEvent::IsTouchEventType(type))
    action_prevented |= touch_start_default_prevented_;

  std::string event_name = WebInputEvent::GetName(type);

  if (latency.source_event_type() == ui::KEY_PRESS)
    event_name = "KeyPress";

  std::string default_action_status =
      action_prevented ? "DefaultPrevented" : "DefaultAllowed";

  LatencyInfo::LatencyComponent main_component;
  if (latency.FindLatency(ui::INPUT_EVENT_LATENCY_RENDERER_MAIN_COMPONENT, 0,
                          &main_component)) {
    DCHECK_EQ(main_component.event_count, 1u);
    if (!multi_finger_touch_gesture) {
      UMA_HISTOGRAM_INPUT_LATENCY_MILLISECONDS(
          "Event.Latency.QueueingTime." + event_name + default_action_status,
          rwh_component, main_component);
    }
  }

  LatencyInfo::LatencyComponent acked_component;
  if (latency.FindLatency(ui::INPUT_EVENT_LATENCY_ACK_RWH_COMPONENT, 0,
                          &acked_component)) {
    DCHECK_EQ(acked_component.event_count, 1u);
    if (!multi_finger_touch_gesture &&
        main_component.event_time != base::TimeTicks()) {
      UMA_HISTOGRAM_INPUT_LATENCY_MILLISECONDS(
          "Event.Latency.BlockingTime." + event_name + default_action_status,
          main_component, acked_component);
    }
  }
}

void RenderWidgetHostLatencyTracker::OnInputEvent(
    const blink::WebInputEvent& event,
    LatencyInfo* latency) {
  DCHECK(latency);
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  OnEventStart(latency);
  if (!set_url_for_ukm_ && render_widget_host_delegate_ &&
      ukm_source_id_ != ukm::kInvalidSourceId) {
    render_widget_host_delegate_->UpdateUrlForUkmSource(ukm::UkmRecorder::Get(),
                                                        ukm_source_id_);
    set_url_for_ukm_ = true;
  }

  if (event.GetType() == WebInputEvent::kTouchStart) {
    const WebTouchEvent& touch_event =
        *static_cast<const WebTouchEvent*>(&event);
    DCHECK(touch_event.touches_length >= 1);
    active_multi_finger_gesture_ = touch_event.touches_length != 1;
  }

  if (latency->source_event_type() == ui::KEY_PRESS) {
    DCHECK(event.GetType() == WebInputEvent::kChar ||
           event.GetType() == WebInputEvent::kRawKeyDown);
  }

  // This is the only place to add the BEGIN_RWH component. So this component
  // should not already be present in the latency info.
  bool found_component =
      latency->FindLatency(ui::INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT,
                           latency_component_id_, nullptr);
  DCHECK(!found_component);

  if (!event.TimeStamp().is_null() &&
      !latency->FindLatency(ui::INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT, 0,
                            nullptr)) {
    base::TimeTicks timestamp_now = base::TimeTicks::Now();
    base::TimeTicks timestamp_original = event.TimeStamp();

    // Timestamp from platform input can wrap, e.g. 32 bits timestamp
    // for Xserver and Window MSG time will wrap about 49.6 days. Do a
    // sanity check here and if wrap does happen, use TimeTicks::Now()
    // as the timestamp instead.
    if ((timestamp_now - timestamp_original).InDays() > 0)
      timestamp_original = timestamp_now;

    latency->AddLatencyNumberWithTimestamp(
        ui::INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT,
        0,
        timestamp_original,
        1);
  }

  latency->AddLatencyNumberWithTraceName(
      ui::INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT, latency_component_id_,
      WebInputEvent::GetName(event.GetType()));

  if (event.GetType() == blink::WebInputEvent::kGestureScrollBegin) {
    has_seen_first_gesture_scroll_update_ = false;
  } else if (event.GetType() == blink::WebInputEvent::kGestureScrollUpdate) {
    // Make a copy of the INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT with a
    // different name INPUT_EVENT_LATENCY_SCROLL_UPDATE_ORIGINAL_COMPONENT.
    // So we can track the latency specifically for scroll update events.
    LatencyInfo::LatencyComponent original_component;
    if (latency->FindLatency(ui::INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT, 0,
                             &original_component)) {
      latency->AddLatencyNumberWithTimestamp(
          has_seen_first_gesture_scroll_update_
              ? ui::INPUT_EVENT_LATENCY_SCROLL_UPDATE_ORIGINAL_COMPONENT
              : ui::INPUT_EVENT_LATENCY_FIRST_SCROLL_UPDATE_ORIGINAL_COMPONENT,
          latency_component_id_, original_component.event_time,
          original_component.event_count);
    }

    has_seen_first_gesture_scroll_update_ = true;
  }
}

void RenderWidgetHostLatencyTracker::OnInputEventAck(
    const blink::WebInputEvent& event,
    LatencyInfo* latency, InputEventAckState ack_result) {
  DCHECK(latency);

  // Latency ends if an event is acked but does not cause render scheduling.
  bool rendering_scheduled = latency->FindLatency(
      ui::INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_MAIN_COMPONENT, 0, nullptr);
  rendering_scheduled |= latency->FindLatency(
      ui::INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_IMPL_COMPONENT, 0, nullptr);

  if (WebInputEvent::IsTouchEventType(event.GetType())) {
    const WebTouchEvent& touch_event =
        *static_cast<const WebTouchEvent*>(&event);
    if (event.GetType() == WebInputEvent::kTouchStart) {
      touch_start_default_prevented_ =
          ack_result == INPUT_EVENT_ACK_STATE_CONSUMED;
    } else if (event.GetType() == WebInputEvent::kTouchEnd ||
               event.GetType() == WebInputEvent::kTouchCancel) {
      active_multi_finger_gesture_ = touch_event.touches_length > 2;
    }
  }

  latency->AddLatencyNumber(ui::INPUT_EVENT_LATENCY_ACK_RWH_COMPONENT, 0);
  // If this event couldn't have caused a gesture event, and it didn't trigger
  // rendering, we're done processing it. If the event got coalesced then
  // terminate it as well.
  if (!rendering_scheduled || latency->coalesced()) {
    latency->AddLatencyNumber(
        ui::INPUT_EVENT_LATENCY_TERMINATED_NO_SWAP_COMPONENT, 0);
  }

  ComputeInputLatencyHistograms(event.GetType(), latency_component_id_,
                                *latency, ack_result);
}

void RenderWidgetHostLatencyTracker::OnEventStart(ui::LatencyInfo* latency) {
  static uint64_t global_trace_id = 0;
  latency->set_trace_id(++global_trace_id);
  latency->set_ukm_source_id(ukm_source_id_);
}

}  // namespace content
