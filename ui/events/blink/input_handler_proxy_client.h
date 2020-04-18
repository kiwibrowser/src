// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_BLINK_INPUT_HANDLER_PROXY_CLIENT_H_
#define UI_EVENTS_BLINK_INPUT_HANDLER_PROXY_CLIENT_H_

#include "ui/events/blink/web_input_event_traits.h"

namespace blink {
class WebGestureCurve;
struct WebFloatPoint;
struct WebSize;
}

namespace ui {

// All callbacks invoked from the compositor thread.
class InputHandlerProxyClient {
 public:
  // Called just before the InputHandlerProxy shuts down.
  virtual void WillShutdown() = 0;

  // Dispatch a non blocking event to the main thread. This is used when a
  // gesture fling from a touchpad is processed and the target only has
  // passive event listeners.
  virtual void DispatchNonBlockingEventToMainThread(
      WebScopedInputEvent event,
      const ui::LatencyInfo& latency_info) = 0;

  // Creates a new fling animation curve instance for device |device_source|
  // with |velocity| and already scrolled |cumulative_scroll| pixels.
  virtual std::unique_ptr<blink::WebGestureCurve> CreateFlingAnimationCurve(
      blink::WebGestureDevice device_source,
      const blink::WebFloatPoint& velocity,
      const blink::WebSize& cumulative_scroll) = 0;

  // |HandleInputEvent/WithLatencyInfo| will respond to overscroll by calling
  // the passed in callback.
  // Otherwise |DidOverscroll| will be fired.
  virtual void DidOverscroll(
      const gfx::Vector2dF& accumulated_overscroll,
      const gfx::Vector2dF& latest_overscroll_delta,
      const gfx::Vector2dF& current_fling_velocity,
      const gfx::PointF& causal_event_viewport_point,
      const cc::OverscrollBehavior& overscroll_behavior) = 0;

  virtual void DidStopFlinging() = 0;

  virtual void DidAnimateForInput() = 0;

  virtual void DidStartScrollingViewport() = 0;

  // Used to send a GSB to the main thread when the wheel scroll latching is
  // enabled and the scrolling should switch to the main thread.
  virtual void GenerateScrollBeginAndSendToMainThread(
      const blink::WebGestureEvent& update_event) = 0;

  virtual void SetWhiteListedTouchAction(
      cc::TouchAction touch_action,
      uint32_t unique_touch_event_id,
      InputHandlerProxy::EventDisposition event_disposition) = 0;

 protected:
  virtual ~InputHandlerProxyClient() {}
};

}  // namespace ui

#endif  // UI_EVENTS_BLINK_INPUT_HANDLER_PROXY_CLIENT_H_
