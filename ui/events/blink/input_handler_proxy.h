// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_BLINK_INPUT_HANDLER_PROXY_H_
#define UI_EVENTS_BLINK_INPUT_HANDLER_PROXY_H_

#include <memory>

#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "cc/input/input_handler.h"
#include "cc/input/snap_fling_controller.h"
#include "third_party/blink/public/platform/web_gesture_curve.h"
#include "third_party/blink/public/platform/web_gesture_curve_target.h"
#include "third_party/blink/public/platform/web_gesture_event.h"
#include "third_party/blink/public/web/web_active_fling_parameters.h"
#include "ui/events/blink/blink_features.h"
#include "ui/events/blink/input_scroll_elasticity_controller.h"
#include "ui/events/blink/synchronous_input_handler_proxy.h"
#include "ui/events/blink/web_input_event_traits.h"

namespace base {
class TickClock;
}

namespace blink {
class WebMouseWheelEvent;
class WebTouchEvent;
}

namespace ui {

namespace test {
class InputHandlerProxyTest;
class InputHandlerProxyEventQueueTest;
class TestInputHandlerProxy;
}

class CompositorThreadEventQueue;
class EventWithCallback;
class FlingBooster;
class InputHandlerProxyClient;
class InputScrollElasticityController;
class SynchronousInputHandler;
class SynchronousInputHandlerProxy;
struct DidOverscrollParams;

// This class is a proxy between the blink web input events for a WebWidget and
// the compositor's input handling logic. InputHandlerProxy instances live
// entirely on the compositor thread. Each InputHandler instance handles input
// events intended for a specific WebWidget.
class InputHandlerProxy : public cc::InputHandlerClient,
                          public SynchronousInputHandlerProxy,
                          public blink::WebGestureCurveTarget,
                          public cc::SnapFlingClient {
 public:
  InputHandlerProxy(cc::InputHandler* input_handler,
                    InputHandlerProxyClient* client,
                    bool touchpad_and_wheel_scroll_latching_enabled,
                    bool async_wheel_events_enabled);
  ~InputHandlerProxy() override;

  InputScrollElasticityController* scroll_elasticity_controller() {
    return scroll_elasticity_controller_.get();
  }

  void set_smooth_scroll_enabled(bool value) { smooth_scroll_enabled_ = value; }

  enum EventDisposition {
    DID_HANDLE,
    DID_NOT_HANDLE,
    DID_NOT_HANDLE_NON_BLOCKING_DUE_TO_FLING,
    DID_HANDLE_NON_BLOCKING,
    DROP_EVENT,
    // The compositor did handle the scroll event (so it wouldn't forward the
    // event to the main thread.) but it didn't consume the scroll so it should
    // pass it to the next consumer (either overscrolling or bubbling the event
    // to the next renderer).
    DID_HANDLE_SHOULD_BUBBLE,
  };
  using EventDispositionCallback =
      base::OnceCallback<void(EventDisposition,
                              WebScopedInputEvent WebInputEvent,
                              const LatencyInfo&,
                              std::unique_ptr<ui::DidOverscrollParams>)>;
  void HandleInputEventWithLatencyInfo(WebScopedInputEvent event,
                                       const LatencyInfo& latency_info,
                                       EventDispositionCallback callback);
  EventDisposition HandleInputEvent(const blink::WebInputEvent& event);

  // cc::InputHandlerClient implementation.
  void WillShutdown() override;
  void Animate(base::TimeTicks time) override;
  void MainThreadHasStoppedFlinging() override;
  void ReconcileElasticOverscrollAndRootScroll() override;
  void UpdateRootLayerStateForSynchronousInputHandler(
      const gfx::ScrollOffset& total_scroll_offset,
      const gfx::ScrollOffset& max_scroll_offset,
      const gfx::SizeF& scrollable_size,
      float page_scale_factor,
      float min_page_scale_factor,
      float max_page_scale_factor) override;
  void DeliverInputForBeginFrame() override;

  // SynchronousInputHandlerProxy implementation.
  void SetOnlySynchronouslyAnimateRootFlings(
      SynchronousInputHandler* synchronous_input_handler) override;
  void SynchronouslyAnimate(base::TimeTicks time) override;
  void SynchronouslySetRootScrollOffset(
      const gfx::ScrollOffset& root_offset) override;
  void SynchronouslyZoomBy(float magnify_delta,
                           const gfx::Point& anchor) override;

  // blink::WebGestureCurveTarget implementation.
  bool ScrollBy(const blink::WebFloatSize& offset,
                const blink::WebFloatSize& velocity) override;

  // SnapFlingClient implementation.
  bool GetSnapFlingInfo(const gfx::Vector2dF& natural_displacement,
                        gfx::Vector2dF* initial_offset,
                        gfx::Vector2dF* target_offset) const override;
  gfx::Vector2dF ScrollByForSnapFling(const gfx::Vector2dF& delta) override;
  void ScrollEndForSnapFling() override;
  void RequestAnimationForSnapFling() override;

  bool gesture_scroll_on_impl_thread_for_testing() const {
    return gesture_scroll_on_impl_thread_;
  }

 protected:
  void RecordMainThreadScrollingReasons(blink::WebGestureDevice device,
                                        uint32_t reasons);
  void RecordScrollingThreadStatus(blink::WebGestureDevice device,
                                   uint32_t reasons);

 private:
  friend class test::TestInputHandlerProxy;
  friend class test::InputHandlerProxyTest;
  friend class test::InputHandlerProxyEventQueueTest;

  void DispatchSingleInputEvent(std::unique_ptr<EventWithCallback>,
                                const base::TimeTicks);
  void DispatchQueuedInputEvents();

  // Helper functions for handling more complicated input events.
  EventDisposition HandleMouseWheel(
      const blink::WebMouseWheelEvent& event);
  EventDisposition HandleGestureScrollBegin(
      const blink::WebGestureEvent& event);
  EventDisposition HandleGestureScrollUpdate(
      const blink::WebGestureEvent& event);
  EventDisposition HandleGestureScrollEnd(
      const blink::WebGestureEvent& event);
  EventDisposition HandleGestureFlingStart(
      const blink::WebGestureEvent& event);
  EventDisposition HandleTouchStart(const blink::WebTouchEvent& event);
  EventDisposition HandleTouchMove(const blink::WebTouchEvent& event);
  EventDisposition HandleTouchEnd(const blink::WebTouchEvent& event);

  // Returns true if we actually had an active fling to cancel, also notifying
  // the client that the fling has ended. Note that if a boosted fling is active
  // and suppressing an active scroll sequence, a synthetic GestureScrollBegin
  // will be injected to resume scrolling.
  bool CancelCurrentFling();

  // Returns true if we actually had an active fling to cancel.
  bool CancelCurrentFlingWithoutNotifyingClient();

  // Request a frame of animation from the InputHandler or
  // SynchronousInputHandler. They can provide that by calling Animate().
  void RequestAnimation();

  // Used to send overscroll messages to the browser.
  // |bundle_overscroll_params_with_ack| means overscroll message should be
  // bundled with triggering event response, and won't fire |DidOverscroll|.
  void HandleOverscroll(const gfx::PointF& causal_event_viewport_point,
                        const cc::InputHandlerScrollResult& scroll_result,
                        bool bundle_overscroll_params_with_ack);

  // Whether to use a smooth scroll animation for this event.
  bool ShouldAnimate(bool has_precise_scroll_deltas) const;

  // Update the elastic overscroll controller with |gesture_event|.
  void HandleScrollElasticityOverscroll(
      const blink::WebGestureEvent& gesture_event,
      const cc::InputHandlerScrollResult& scroll_result);

  // Overrides the internal clock for testing.
  // This doesn't take the ownership of the clock. |tick_clock| must outlive the
  // InputHandlerProxy instance.
  void SetTickClockForTesting(const base::TickClock* tick_clock);

  // |is_touching_scrolling_layer| indicates if one of the points that has
  // been touched hits a currently scrolling layer.
  // |white_listed_touch_action| is the touch_action we are sure will be
  // allowed for the given touch event.
  EventDisposition HitTestTouchEvent(
      const blink::WebTouchEvent& touch_event,
      bool* is_touching_scrolling_layer,
      cc::TouchAction* white_listed_touch_action);

  void UpdateCurrentFlingState(const blink::WebGestureEvent& fling_start_event,
                               const gfx::Vector2dF& velocity);

  std::unique_ptr<blink::WebGestureCurve> fling_curve_;
  // Parameters for the active fling animation, stored in case we need to
  // transfer it out later.
  blink::WebActiveFlingParameters fling_parameters_;

  InputHandlerProxyClient* client_;
  cc::InputHandler* input_handler_;

  // When present, Animates are not requested to the InputHandler, but to this
  // SynchronousInputHandler instead. And all Animate() calls are expected to
  // happen via the SynchronouslyAnimate() call instead of coming directly from
  // the InputHandler.
  SynchronousInputHandler* synchronous_input_handler_;
  bool allow_root_animate_;

#ifndef NDEBUG
  bool expect_scroll_update_end_;
#endif
  bool gesture_scroll_on_impl_thread_;
  bool gesture_pinch_on_impl_thread_;
  bool in_inertial_scrolling_ = false;
  bool scroll_sequence_ignored_;
  // This is always false when there are no flings on the main thread, but
  // conservative in the sense that we might not be actually flinging when it is
  // true.
  bool fling_may_be_active_on_main_thread_;
  // The axes on which the current fling is allowed to scroll.  If a given fling
  // has overscrolled on a particular axis, further fling scrolls on that axis
  // will be disabled.
  bool disallow_horizontal_fling_scroll_;
  bool disallow_vertical_fling_scroll_;

  // Whether an active fling has seen an |Animate()| call. This is useful for
  // determining if the fling start time should be re-initialized.
  bool has_fling_animation_started_;

  // Non-zero only within the scope of |scrollBy|.
  gfx::Vector2dF current_fling_velocity_;

  // Used to animate rubber-band over-scroll effect on Mac.
  std::unique_ptr<InputScrollElasticityController>
      scroll_elasticity_controller_;

  bool smooth_scroll_enabled_;
  const bool touchpad_and_wheel_scroll_latching_enabled_;
  const bool async_wheel_events_enabled_;

  // The merged result of the last touch event with previous touch events.
  // This value will get returned for subsequent TouchMove events to allow
  // passive events not to block scrolling.
  int32_t touch_result_;

  // The result of the last mouse wheel event. This value is used to determine
  // whether the next wheel scroll is blocked on the Main thread or not.
  int32_t mouse_wheel_result_;

  base::TimeTicks last_fling_animate_time_;

  // Used to record overscroll notifications while an event is being
  // dispatched.  If the event causes overscroll, the overscroll metadata can be
  // bundled in the event ack, saving an IPC.  Note that we must continue
  // supporting overscroll IPC notifications due to fling animation updates.
  std::unique_ptr<DidOverscrollParams> current_overscroll_params_;

  std::unique_ptr<CompositorThreadEventQueue> compositor_event_queue_;
  bool has_ongoing_compositor_scroll_fling_pinch_;
  bool is_first_gesture_scroll_update_;

  const base::TickClock* tick_clock_;

  std::unique_ptr<FlingBooster> fling_booster_;

  std::unique_ptr<cc::SnapFlingController> snap_fling_controller_;

  DISALLOW_COPY_AND_ASSIGN(InputHandlerProxy);
};

}  // namespace ui

#endif  // UI_EVENTS_BLINK_INPUT_HANDLER_PROXY_H_
