// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/input/input_event_ack.h"

#include <utility>

namespace content {

InputEventAck::InputEventAck(
    InputEventAckSource source,
    blink::WebInputEvent::Type type,
    InputEventAckState state,
    const ui::LatencyInfo& latency,
    std::unique_ptr<ui::DidOverscrollParams> overscroll,
    uint32_t unique_touch_event_id,
    base::Optional<cc::TouchAction> touch_action)
    : source(source),
      type(type),
      state(state),
      latency(latency),
      overscroll(std::move(overscroll)),
      unique_touch_event_id(unique_touch_event_id),
      touch_action(touch_action) {}

InputEventAck::InputEventAck(InputEventAckSource source,
                             blink::WebInputEvent::Type type,
                             InputEventAckState state,
                             const ui::LatencyInfo& latency,
                             uint32_t unique_touch_event_id)
    : InputEventAck(source,
                    type,
                    state,
                    latency,
                    nullptr,
                    unique_touch_event_id,
                    base::nullopt) {}

InputEventAck::InputEventAck(InputEventAckSource source,
                             blink::WebInputEvent::Type type,
                             InputEventAckState state,
                             uint32_t unique_touch_event_id)
    : InputEventAck(source,
                    type,
                    state,
                    ui::LatencyInfo(),
                    unique_touch_event_id) {}

InputEventAck::InputEventAck(InputEventAckSource source,
                             blink::WebInputEvent::Type type,
                             InputEventAckState state)
    : InputEventAck(source, type, state, 0) {}

InputEventAck::InputEventAck()
    : InputEventAck(InputEventAckSource::UNKNOWN,
                    blink::WebInputEvent::kUndefined,
                    INPUT_EVENT_ACK_STATE_UNKNOWN) {}

InputEventAck::~InputEventAck() {
}

}  // namespace content
