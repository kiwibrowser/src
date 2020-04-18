// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/synthetic_gesture_target_base.h"

#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/renderer_host/ui_events_helper.h"
#include "content/common/input_messages.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "ui/events/blink/web_input_event_traits.h"
#include "ui/events/event.h"
#include "ui/latency/latency_info.h"

using blink::WebInputEvent;
using blink::WebTouchEvent;
using blink::WebTouchPoint;
using blink::WebMouseEvent;
using blink::WebMouseWheelEvent;
using blink::WebGestureEvent;

namespace content {
namespace {

// This value was determined experimentally. It was sufficient to not cause a
// fling on Android and Aura.
const int kPointerAssumedStoppedTimeMs = 100;

// SyntheticGestureTargetBase passes input events straight on to the renderer
// without going through a gesture recognition framework. There is thus no touch
// slop.
const float kTouchSlopInDips = 0.0f;

}  // namespace

SyntheticGestureTargetBase::SyntheticGestureTargetBase(
    RenderWidgetHostImpl* host)
    : host_(host) {
  DCHECK(host);
}

SyntheticGestureTargetBase::~SyntheticGestureTargetBase() {
}

void SyntheticGestureTargetBase::DispatchInputEventToPlatform(
    const WebInputEvent& event) {
  TRACE_EVENT1("input", "SyntheticGestureTarget::DispatchInputEventToPlatform",
               "type", WebInputEvent::GetName(event.GetType()));

  ui::LatencyInfo latency_info;
  latency_info.AddLatencyNumber(ui::INPUT_EVENT_LATENCY_UI_COMPONENT, 0);

  if (WebInputEvent::IsTouchEventType(event.GetType())) {
    const WebTouchEvent& web_touch =
        static_cast<const WebTouchEvent&>(event);

    // Check that all touch pointers are within the content bounds.
    for (unsigned i = 0; i < web_touch.touches_length; i++) {
      if (web_touch.touches[i].state == WebTouchPoint::kStatePressed &&
          !PointIsWithinContents(web_touch.touches[i].PositionInWidget().x,
                                 web_touch.touches[i].PositionInWidget().y)) {
        LOG(WARNING)
            << "Touch coordinates are not within content bounds on TouchStart.";
        return;
      }
    }
    DispatchWebTouchEventToPlatform(web_touch, latency_info);
  } else if (event.GetType() == WebInputEvent::kMouseWheel) {
    const WebMouseWheelEvent& web_wheel =
        static_cast<const WebMouseWheelEvent&>(event);
    if (!PointIsWithinContents(web_wheel.PositionInWidget().x,
                               web_wheel.PositionInWidget().y)) {
      LOG(WARNING) << "Mouse wheel position is not within content bounds.";
      return;
    }
    DispatchWebMouseWheelEventToPlatform(web_wheel, latency_info);
  } else if (WebInputEvent::IsMouseEventType(event.GetType())) {
    const WebMouseEvent& web_mouse =
        static_cast<const WebMouseEvent&>(event);
    if (event.GetType() == WebInputEvent::kMouseDown &&
        !PointIsWithinContents(web_mouse.PositionInWidget().x,
                               web_mouse.PositionInWidget().y)) {
      LOG(WARNING)
          << "Mouse pointer is not within content bounds on MouseDown.";
      return;
    }
    DispatchWebMouseEventToPlatform(web_mouse, latency_info);
  } else if (WebInputEvent::IsPinchGestureEventType(event.GetType())) {
    const WebGestureEvent& web_pinch =
        static_cast<const WebGestureEvent&>(event);
    // Touchscreen pinches should be injected as touch events.
    DCHECK_EQ(blink::kWebGestureDeviceTouchpad, web_pinch.SourceDevice());
    if (event.GetType() == WebInputEvent::kGesturePinchBegin &&
        !PointIsWithinContents(web_pinch.PositionInWidget().x,
                               web_pinch.PositionInWidget().y)) {
      LOG(WARNING)
          << "Pinch coordinates are not within content bounds on PinchBegin.";
      return;
    }
    DispatchWebGestureEventToPlatform(web_pinch, latency_info);
  } else {
    NOTREACHED();
  }
}

void SyntheticGestureTargetBase::DispatchWebTouchEventToPlatform(
      const blink::WebTouchEvent& web_touch,
      const ui::LatencyInfo& latency_info) {
  // We assume that platforms supporting touch have their own implementation of
  // SyntheticGestureTarget to route the events through their respective input
  // stack.
  LOG(ERROR) << "Touch events not supported for this browser.";
}

void SyntheticGestureTargetBase::DispatchWebMouseWheelEventToPlatform(
      const blink::WebMouseWheelEvent& web_wheel,
      const ui::LatencyInfo& latency_info) {
  host_->ForwardWheelEventWithLatencyInfo(web_wheel, latency_info);
}

void SyntheticGestureTargetBase::DispatchWebGestureEventToPlatform(
    const blink::WebGestureEvent& web_gesture,
    const ui::LatencyInfo& latency_info) {
  host_->ForwardGestureEventWithLatencyInfo(web_gesture, latency_info);
}

void SyntheticGestureTargetBase::DispatchWebMouseEventToPlatform(
      const blink::WebMouseEvent& web_mouse,
      const ui::LatencyInfo& latency_info) {
  host_->ForwardMouseEventWithLatencyInfo(web_mouse, latency_info);
}

SyntheticGestureParams::GestureSourceType
SyntheticGestureTargetBase::GetDefaultSyntheticGestureSourceType() const {
  return SyntheticGestureParams::MOUSE_INPUT;
}

base::TimeDelta SyntheticGestureTargetBase::PointerAssumedStoppedTime()
    const {
  return base::TimeDelta::FromMilliseconds(kPointerAssumedStoppedTimeMs);
}

float SyntheticGestureTargetBase::GetTouchSlopInDips() const {
  return kTouchSlopInDips;
}

float SyntheticGestureTargetBase::GetSpanSlopInDips() const {
  // * 2 because span is the distance between two touch points in a pinch-zoom
  // gesture so we're accounting for movement in two points.
  return 2.f * GetTouchSlopInDips();
}

float SyntheticGestureTargetBase::GetMinScalingSpanInDips() const {
  // The minimum scaling distance is only relevant for touch gestures and the
  // base target doesn't support touch.
  NOTREACHED();
  return 0.0f;
}

int SyntheticGestureTargetBase::GetMouseWheelMinimumGranularity() const {
  return host_->GetView()->GetMouseWheelMinimumGranularity();
}

bool SyntheticGestureTargetBase::PointIsWithinContents(int x, int y) const {
  gfx::Rect bounds = host_->GetView()->GetViewBounds();
  bounds -= bounds.OffsetFromOrigin();  // Translate the bounds to (0,0).
  return bounds.Contains(x, y);
}

}  // namespace content
