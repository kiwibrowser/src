// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/blink/input_handler_proxy.h"

#include <stddef.h>

#include <algorithm>

#include "base/auto_reset.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_tick_clock.h"
#include "base/trace_event/trace_event.h"
#include "cc/input/main_thread_scrolling_reason.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/public/platform/web_mouse_wheel_event.h"
#include "third_party/blink/public/platform/web_touch_event.h"
#include "ui/events/blink/blink_event_util.h"
#include "ui/events/blink/compositor_thread_event_queue.h"
#include "ui/events/blink/did_overscroll_params.h"
#include "ui/events/blink/event_with_callback.h"
#include "ui/events/blink/fling_booster.h"
#include "ui/events/blink/input_handler_proxy_client.h"
#include "ui/events/blink/input_scroll_elasticity_controller.h"
#include "ui/events/blink/web_input_event_traits.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/latency/latency_info.h"

using blink::WebFloatPoint;
using blink::WebFloatSize;
using blink::WebGestureEvent;
using blink::WebInputEvent;
using blink::WebMouseEvent;
using blink::WebMouseWheelEvent;
using blink::WebPoint;
using blink::WebTouchEvent;
using blink::WebTouchPoint;

namespace {

const int32_t kEventDispositionUndefined = -1;

// Maximum time between a fling event's timestamp and the first |Animate| call
// for the fling curve to use the fling timestamp as the initial animation time.
// Two frames allows a minor delay between event creation and the first animate.
constexpr base::TimeDelta kMaxIntervalFromFlingTimestampToFirstAnimate =
    base::TimeDelta::FromSecondsD(2. / 60.);

// Threshold for determining whether a fling scroll delta should have caused the
// client to scroll.
const float kScrollEpsilon = 0.1f;

const size_t kTenSeconds = 10 * 1000 * 1000;

gfx::Vector2dF ToClientScrollIncrement(const WebFloatSize& increment) {
  return gfx::Vector2dF(-increment.width, -increment.height);
}

double InSecondsF(const base::TimeTicks& time) {
  return (time - base::TimeTicks()).InSecondsF();
}

WebGestureEvent ObtainGestureScrollBegin(const WebGestureEvent& event) {
  WebGestureEvent scroll_begin_event = event;
  scroll_begin_event.SetType(WebInputEvent::kGestureScrollBegin);
  scroll_begin_event.data.scroll_begin.delta_x_hint = 0;
  scroll_begin_event.data.scroll_begin.delta_y_hint = 0;
  return scroll_begin_event;
}

cc::ScrollState CreateScrollStateForGesture(const WebGestureEvent& event) {
  cc::ScrollStateData scroll_state_data;
  switch (event.GetType()) {
    case WebInputEvent::kGestureScrollBegin:
      scroll_state_data.position_x = event.PositionInWidget().x;
      scroll_state_data.position_y = event.PositionInWidget().y;
      scroll_state_data.delta_x_hint = -event.data.scroll_begin.delta_x_hint;
      scroll_state_data.delta_y_hint = -event.data.scroll_begin.delta_y_hint;
      scroll_state_data.is_beginning = true;
      // On Mac, a GestureScrollBegin in the inertial phase indicates a fling
      // start.
      scroll_state_data.is_in_inertial_phase =
          (event.data.scroll_begin.inertial_phase ==
           WebGestureEvent::kMomentumPhase);
      break;
    case WebInputEvent::kGestureFlingStart:
      scroll_state_data.velocity_x = event.data.fling_start.velocity_x;
      scroll_state_data.velocity_y = event.data.fling_start.velocity_y;
      scroll_state_data.is_in_inertial_phase = true;
      break;
    case WebInputEvent::kGestureScrollUpdate:
      scroll_state_data.delta_x = -event.data.scroll_update.delta_x;
      scroll_state_data.delta_y = -event.data.scroll_update.delta_y;
      scroll_state_data.velocity_x = event.data.scroll_update.velocity_x;
      scroll_state_data.velocity_y = event.data.scroll_update.velocity_y;
      scroll_state_data.is_in_inertial_phase =
          event.data.scroll_update.inertial_phase ==
          WebGestureEvent::kMomentumPhase;
      break;
    case WebInputEvent::kGestureScrollEnd:
    case WebInputEvent::kGestureFlingCancel:
      scroll_state_data.is_ending = true;
      break;
    default:
      NOTREACHED();
      break;
  }
  return cc::ScrollState(scroll_state_data);
}

cc::ScrollState CreateScrollStateForInertialEnd() {
  cc::ScrollStateData scroll_state_data;
  scroll_state_data.is_ending = true;
  return cc::ScrollState(scroll_state_data);
}

cc::ScrollState CreateScrollStateForInertialUpdate(
    const gfx::Vector2dF& delta) {
  cc::ScrollStateData scroll_state_data;
  scroll_state_data.delta_x = delta.x();
  scroll_state_data.delta_y = delta.y();
  scroll_state_data.is_in_inertial_phase = true;
  return cc::ScrollState(scroll_state_data);
}

cc::InputHandler::ScrollInputType GestureScrollInputType(
    blink::WebGestureDevice device) {
  return device == blink::kWebGestureDeviceTouchpad
             ? cc::InputHandler::WHEEL
             : cc::InputHandler::TOUCHSCREEN;
}

cc::SnapFlingController::GestureScrollType GestureScrollEventType(
    WebInputEvent::Type web_event_type) {
  switch (web_event_type) {
    case WebInputEvent::kGestureScrollBegin:
      return cc::SnapFlingController::GestureScrollType::kBegin;
    case WebInputEvent::kGestureScrollUpdate:
      return cc::SnapFlingController::GestureScrollType::kUpdate;
    case WebInputEvent::kGestureScrollEnd:
      return cc::SnapFlingController::GestureScrollType::kEnd;
    default:
      NOTREACHED();
      return cc::SnapFlingController::GestureScrollType::kBegin;
  }
}

cc::SnapFlingController::GestureScrollUpdateInfo GetGestureScrollUpdateInfo(
    const WebGestureEvent& event) {
  cc::SnapFlingController::GestureScrollUpdateInfo info;
  info.delta = gfx::Vector2dF(-event.data.scroll_update.delta_x,
                              -event.data.scroll_update.delta_y);
  info.is_in_inertial_phase = event.data.scroll_update.inertial_phase ==
                              blink::WebGestureEvent::kMomentumPhase;
  info.event_time = event.TimeStamp();
  return info;
}

enum ScrollingThreadStatus {
  SCROLLING_ON_COMPOSITOR,
  SCROLLING_ON_COMPOSITOR_BLOCKED_ON_MAIN,
  SCROLLING_ON_MAIN,
  LAST_SCROLLING_THREAD_STATUS_VALUE = SCROLLING_ON_MAIN,
};

}  // namespace

namespace ui {

InputHandlerProxy::InputHandlerProxy(
    cc::InputHandler* input_handler,
    InputHandlerProxyClient* client,
    bool touchpad_and_wheel_scroll_latching_enabled,
    bool async_wheel_events_enabled)
    : client_(client),
      input_handler_(input_handler),
      synchronous_input_handler_(nullptr),
      allow_root_animate_(true),
#ifndef NDEBUG
      expect_scroll_update_end_(false),
#endif
      gesture_scroll_on_impl_thread_(false),
      gesture_pinch_on_impl_thread_(false),
      scroll_sequence_ignored_(false),
      fling_may_be_active_on_main_thread_(false),
      disallow_horizontal_fling_scroll_(false),
      disallow_vertical_fling_scroll_(false),
      has_fling_animation_started_(false),
      smooth_scroll_enabled_(false),
      touchpad_and_wheel_scroll_latching_enabled_(
          touchpad_and_wheel_scroll_latching_enabled),
      async_wheel_events_enabled_(touchpad_and_wheel_scroll_latching_enabled &&
                                  async_wheel_events_enabled),
      touch_result_(kEventDispositionUndefined),
      mouse_wheel_result_(kEventDispositionUndefined),
      current_overscroll_params_(nullptr),
      has_ongoing_compositor_scroll_fling_pinch_(false),
      is_first_gesture_scroll_update_(false),
      tick_clock_(base::DefaultTickClock::GetInstance()),
      snap_fling_controller_(std::make_unique<cc::SnapFlingController>(this)) {
  DCHECK(client);
  input_handler_->BindToClient(this,
                               touchpad_and_wheel_scroll_latching_enabled_);
  cc::ScrollElasticityHelper* scroll_elasticity_helper =
      input_handler_->CreateScrollElasticityHelper();
  if (scroll_elasticity_helper) {
    scroll_elasticity_controller_.reset(
        new InputScrollElasticityController(scroll_elasticity_helper));
  }
  compositor_event_queue_ =
      base::FeatureList::IsEnabled(features::kVsyncAlignedInputEvents)
          ? std::make_unique<CompositorThreadEventQueue>()
          : nullptr;
}

InputHandlerProxy::~InputHandlerProxy() {}

void InputHandlerProxy::WillShutdown() {
  scroll_elasticity_controller_.reset();
  fling_booster_.reset();
  input_handler_ = NULL;
  client_->WillShutdown();
}

void InputHandlerProxy::HandleInputEventWithLatencyInfo(
    WebScopedInputEvent event,
    const LatencyInfo& latency_info,
    EventDispositionCallback callback) {
  DCHECK(input_handler_);

  TRACE_EVENT_WITH_FLOW1("input,benchmark", "LatencyInfo.Flow",
                         TRACE_ID_DONT_MANGLE(latency_info.trace_id()),
                         TRACE_EVENT_FLAG_FLOW_IN | TRACE_EVENT_FLAG_FLOW_OUT,
                         "step", "HandleInputEventImpl");

  std::unique_ptr<EventWithCallback> event_with_callback =
      std::make_unique<EventWithCallback>(std::move(event), latency_info,
                                          tick_clock_->NowTicks(),
                                          std::move(callback));

  // Note: Other input can race ahead of gesture input as they don't have to go
  // through the queue, but we believe it's OK to do so.
  if (!compositor_event_queue_ ||
      !IsGestureScrollOrFlingOrPinch(event_with_callback->event().GetType())) {
    DispatchSingleInputEvent(std::move(event_with_callback),
                             tick_clock_->NowTicks());
    return;
  }

  if (has_ongoing_compositor_scroll_fling_pinch_) {
    const auto& gesture_event = ToWebGestureEvent(event_with_callback->event());
    bool is_from_set_non_blocking_touch =
        gesture_event.SourceDevice() == blink::kWebGestureDeviceTouchscreen &&
        gesture_event.is_source_touch_event_set_non_blocking;
    bool is_scroll_end_from_wheel =
        gesture_event.SourceDevice() == blink::kWebGestureDeviceTouchpad &&
        gesture_event.GetType() == blink::WebGestureEvent::kGestureScrollEnd;
    bool scroll_update_has_blocking_wheel_source =
        gesture_event.SourceDevice() == blink::kWebGestureDeviceTouchpad &&
        gesture_event.GetType() ==
            blink::WebGestureEvent::kGestureScrollUpdate &&
        (!async_wheel_events_enabled_ || is_first_gesture_scroll_update_);
    if (gesture_event.GetType() ==
        blink::WebGestureEvent::kGestureScrollUpdate) {
      is_first_gesture_scroll_update_ = false;
    }
    if (is_from_set_non_blocking_touch || is_scroll_end_from_wheel ||
        scroll_update_has_blocking_wheel_source || synchronous_input_handler_) {
      // 1. Gesture events was already delayed by blocking events in rAF aligned
      // queue. We want to avoid additional one frame delay by flushing the
      // VSync queue immediately.
      // The first GSU latency was tracked by:
      // |smoothness.tough_scrolling_cases:first_gesture_scroll_update_latency|.
      // 2. |synchronous_input_handler_| is WebView only. WebView has different
      // mechanisms and we want to forward all events immediately.
      compositor_event_queue_->Queue(std::move(event_with_callback),
                                     tick_clock_->NowTicks());
      DispatchQueuedInputEvents();
      return;
    }

    bool needs_animate_input = compositor_event_queue_->empty();
    compositor_event_queue_->Queue(std::move(event_with_callback),
                                   tick_clock_->NowTicks());
    if (needs_animate_input)
      input_handler_->SetNeedsAnimateInput();
    return;
  }

  // We have to dispatch the event to know whether the gesture sequence will be
  // handled by the compositor or not.
  DispatchSingleInputEvent(std::move(event_with_callback),
                           tick_clock_->NowTicks());
}

void InputHandlerProxy::DispatchSingleInputEvent(
    std::unique_ptr<EventWithCallback> event_with_callback,
    const base::TimeTicks now) {
  if (compositor_event_queue_ &&
      IsGestureScrollOrFlingOrPinch(event_with_callback->event().GetType())) {
    // Report the coalesced count only for continuous events to avoid the noise
    // from non-continuous events.
    if (IsContinuousGestureEvent(event_with_callback->event().GetType())) {
      UMA_HISTOGRAM_CUSTOM_COUNTS(
          "Event.CompositorThreadEventQueue.Continuous.HeadQueueingTime",
          (now - event_with_callback->creation_timestamp()).InMicroseconds(), 1,
          kTenSeconds, 50);

      UMA_HISTOGRAM_CUSTOM_COUNTS(
          "Event.CompositorThreadEventQueue.Continuous.TailQueueingTime",
          (now - event_with_callback->last_coalesced_timestamp())
              .InMicroseconds(),
          1, kTenSeconds, 50);

      UMA_HISTOGRAM_COUNTS_1000(
          "Event.CompositorThreadEventQueue.CoalescedCount",
          static_cast<int>(event_with_callback->coalesced_count()));
    } else {
      UMA_HISTOGRAM_CUSTOM_COUNTS(
          "Event.CompositorThreadEventQueue.NonContinuous.QueueingTime",
          (now - event_with_callback->creation_timestamp()).InMicroseconds(), 1,
          kTenSeconds, 50);
    }
  }

  ui::LatencyInfo monitored_latency_info = event_with_callback->latency_info();
  std::unique_ptr<cc::SwapPromiseMonitor> latency_info_swap_promise_monitor =
      input_handler_->CreateLatencyInfoSwapPromiseMonitor(
          &monitored_latency_info);

  current_overscroll_params_.reset();
  InputHandlerProxy::EventDisposition disposition =
      HandleInputEvent(event_with_callback->event());

  switch (event_with_callback->event().GetType()) {
    case blink::WebGestureEvent::kGestureScrollBegin:
      is_first_gesture_scroll_update_ = true;
      FALLTHROUGH;
    case blink::WebGestureEvent::kGestureFlingStart:
    case blink::WebGestureEvent::kGesturePinchBegin:
    case blink::WebGestureEvent::kGestureScrollUpdate:
    case blink::WebGestureEvent::kGesturePinchUpdate:
      has_ongoing_compositor_scroll_fling_pinch_ = disposition == DID_HANDLE;
      break;

    case blink::WebGestureEvent::kGestureScrollEnd:
    case blink::WebGestureEvent::kGestureFlingCancel:
    case blink::WebGestureEvent::kGesturePinchEnd:
      has_ongoing_compositor_scroll_fling_pinch_ = false;
      break;
    default:
      break;
  }

  // Will run callback for every original events.
  event_with_callback->RunCallbacks(disposition, monitored_latency_info,
                                    std::move(current_overscroll_params_));
}

void InputHandlerProxy::DispatchQueuedInputEvents() {
  if (!compositor_event_queue_)
    return;

  // Calling |NowTicks()| is expensive so we only want to do it once.
  base::TimeTicks now = tick_clock_->NowTicks();
  while (!compositor_event_queue_->empty())
    DispatchSingleInputEvent(compositor_event_queue_->Pop(), now);
}

InputHandlerProxy::EventDisposition InputHandlerProxy::HandleInputEvent(
    const WebInputEvent& event) {
  DCHECK(input_handler_);

  if (fling_booster_ && WebInputEvent::IsGestureEventType(event.GetType())) {
    const WebGestureEvent& gesture_event =
        static_cast<const WebGestureEvent&>(event);
    bool cancel_current_fling;
    bool should_filter_event =
        fling_booster_->FilterGestureEventForFlingBoosting(
            gesture_event, &cancel_current_fling);
    if (cancel_current_fling)
      CancelCurrentFling();

    if (should_filter_event) {
      if (event.GetType() == WebInputEvent::kGestureFlingStart) {
        UpdateCurrentFlingState(gesture_event,
                                fling_booster_->current_fling_velocity());
        TRACE_EVENT_INSTANT2("input",
                             fling_booster_->fling_boosted()
                                 ? "InputHandlerProxy::FlingBoosted"
                                 : "InputHandlerProxy::FlingReplaced",
                             TRACE_EVENT_SCOPE_THREAD, "vx",
                             current_fling_velocity_.x(), "vy",
                             current_fling_velocity_.y());

        // The client expects balanced calls between a consumed
        // GestureFlingStart and |DidStopFlinging()|.
        client_->DidStopFlinging();
      } else if (event.GetType() == WebInputEvent::kGestureFlingCancel) {
        DCHECK(fling_booster_->fling_cancellation_is_deferred());
        TRACE_EVENT_INSTANT0("input", "InputHandlerProxy::FlingBoostStart",
                             TRACE_EVENT_SCOPE_THREAD);
      } else if (event.GetType() == WebInputEvent::kGestureScrollBegin ||
                 event.GetType() == WebInputEvent::kGestureScrollUpdate) {
        TRACE_EVENT_INSTANT0("input",
                             "InputHandlerProxy::ExtendBoostedFlingTimeout",
                             TRACE_EVENT_SCOPE_THREAD);
      }

      return DID_HANDLE;
    }
  }

  if (IsGestureScroll(event.GetType()) &&
      (snap_fling_controller_->FilterEventForSnap(
          GestureScrollEventType(event.GetType())))) {
    return DROP_EVENT;
  }

  switch (event.GetType()) {
    case WebInputEvent::kMouseWheel:
      return HandleMouseWheel(static_cast<const WebMouseWheelEvent&>(event));

    case WebInputEvent::kGestureScrollBegin:
      return HandleGestureScrollBegin(
          static_cast<const WebGestureEvent&>(event));

    case WebInputEvent::kGestureScrollUpdate:
      return HandleGestureScrollUpdate(
          static_cast<const WebGestureEvent&>(event));

    case WebInputEvent::kGestureScrollEnd:
      return HandleGestureScrollEnd(static_cast<const WebGestureEvent&>(event));

    case WebInputEvent::kGesturePinchBegin: {
      DCHECK(!gesture_pinch_on_impl_thread_);
      const WebGestureEvent& gesture_event =
          static_cast<const WebGestureEvent&>(event);
      if (gesture_event.SourceDevice() == blink::kWebGestureDeviceTouchpad &&
          input_handler_->GetEventListenerProperties(
              cc::EventListenerClass::kMouseWheel) !=
              cc::EventListenerProperties::kNone) {
        return DID_NOT_HANDLE;
      } else {
        input_handler_->PinchGestureBegin();
        gesture_pinch_on_impl_thread_ = true;
        return DID_HANDLE;
      }
    }

    case WebInputEvent::kGesturePinchEnd:
      if (gesture_pinch_on_impl_thread_) {
        gesture_pinch_on_impl_thread_ = false;
        const WebGestureEvent& gesture_event =
            static_cast<const WebGestureEvent&>(event);
        input_handler_->PinchGestureEnd(
            gfx::ToFlooredPoint(gesture_event.PositionInWidget()),
            gesture_event.SourceDevice() == blink::kWebGestureDeviceTouchpad);
        return DID_HANDLE;
      } else {
        return DID_NOT_HANDLE;
      }

    case WebInputEvent::kGesturePinchUpdate: {
      if (gesture_pinch_on_impl_thread_) {
        const WebGestureEvent& gesture_event =
            static_cast<const WebGestureEvent&>(event);
        if (gesture_event.data.pinch_update.zoom_disabled)
          return DROP_EVENT;
        input_handler_->PinchGestureUpdate(
            gesture_event.data.pinch_update.scale,
            gfx::ToFlooredPoint(gesture_event.PositionInWidget()));
        return DID_HANDLE;
      } else {
        return DID_NOT_HANDLE;
      }
    }

    case WebInputEvent::kGestureFlingStart:
      return HandleGestureFlingStart(
          *static_cast<const WebGestureEvent*>(&event));

    case WebInputEvent::kGestureFlingCancel:
      if (CancelCurrentFling())
        return DID_HANDLE;
      else if (!fling_may_be_active_on_main_thread_)
        return DROP_EVENT;
      return DID_NOT_HANDLE;

    case WebInputEvent::kTouchStart:
      return HandleTouchStart(static_cast<const WebTouchEvent&>(event));

    case WebInputEvent::kTouchMove:
      return HandleTouchMove(static_cast<const WebTouchEvent&>(event));

    case WebInputEvent::kTouchEnd:
      return HandleTouchEnd(static_cast<const WebTouchEvent&>(event));

    case WebInputEvent::kMouseDown: {
      // Only for check scrollbar captured
      const WebMouseEvent& mouse_event =
          static_cast<const WebMouseEvent&>(event);

      if (mouse_event.button == blink::WebMouseEvent::Button::kLeft) {
        CHECK(input_handler_);
        input_handler_->MouseDown();
      }
      return DID_NOT_HANDLE;
    }
    case WebInputEvent::kMouseUp: {
      // Only for release scrollbar captured
      const WebMouseEvent& mouse_event =
          static_cast<const WebMouseEvent&>(event);

      if (mouse_event.button == blink::WebMouseEvent::Button::kLeft) {
        CHECK(input_handler_);
        input_handler_->MouseUp();
      }
      return DID_NOT_HANDLE;
    }
    case WebInputEvent::kMouseMove: {
      const WebMouseEvent& mouse_event =
          static_cast<const WebMouseEvent&>(event);
      // TODO(davemoore): This should never happen, but bug #326635 showed some
      // surprising crashes.
      CHECK(input_handler_);
      input_handler_->MouseMoveAt(gfx::Point(mouse_event.PositionInWidget().x,
                                             mouse_event.PositionInWidget().y));
      return DID_NOT_HANDLE;
    }
    case WebInputEvent::kMouseLeave: {
      CHECK(input_handler_);
      input_handler_->MouseLeave();
      return DID_NOT_HANDLE;
    }

    default:
      if (WebInputEvent::IsKeyboardEventType(event.GetType())) {
        // Only call |CancelCurrentFling()| if a fling was active, as it will
        // otherwise disrupt an in-progress touch scroll.
        if (fling_curve_)
          CancelCurrentFling();
      }
      break;
  }

  return DID_NOT_HANDLE;
}

void InputHandlerProxy::RecordMainThreadScrollingReasons(
    blink::WebGestureDevice device,
    uint32_t reasons) {
  static const char* kGestureHistogramName =
      "Renderer4.MainThreadGestureScrollReason";
  static const char* kWheelHistogramName =
      "Renderer4.MainThreadWheelScrollReason";

  if (device != blink::kWebGestureDeviceTouchpad &&
      device != blink::kWebGestureDeviceTouchscreen) {
    return;
  }

  // NonCompositedScrollReasons should only be set on the main thread.
  DCHECK(
      !cc::MainThreadScrollingReason::HasNonCompositedScrollReasons(reasons));

  // UMA_HISTOGRAM_ENUMERATION requires that the enum_max must be strictly
  // greater than the sample value. kMainThreadScrollingReasonCount doesn't
  // include the NotScrollingOnMain enum but the histograms do so adding
  // the +1 is necessary.
  // TODO(dcheng): Fix https://crbug.com/705169 so this isn't needed.
  constexpr uint32_t kMainThreadScrollingReasonEnumMax =
      cc::MainThreadScrollingReason::kMainThreadScrollingReasonCount + 1;
  if (reasons == cc::MainThreadScrollingReason::kNotScrollingOnMain) {
    if (device == blink::kWebGestureDeviceTouchscreen) {
      UMA_HISTOGRAM_ENUMERATION(
          kGestureHistogramName,
          cc::MainThreadScrollingReason::kNotScrollingOnMain,
          kMainThreadScrollingReasonEnumMax);
    } else {
      UMA_HISTOGRAM_ENUMERATION(
          kWheelHistogramName,
          cc::MainThreadScrollingReason::kNotScrollingOnMain,
          kMainThreadScrollingReasonEnumMax);
    }
  }

  for (uint32_t i = 0;
       i < cc::MainThreadScrollingReason::kMainThreadScrollingReasonCount;
       ++i) {
    unsigned val = 1 << i;
    if (reasons & val) {
      if (val == cc::MainThreadScrollingReason::kHandlingScrollFromMainThread) {
        // We only want to record "Handling scroll from main thread" reason if
        // it's the only reason. If it's not the only reason, the "real" reason
        // for scrolling on main is something else, and we only want to pay
        // attention to that reason.
        if (reasons & ~val)
          continue;
      }
      if (device == blink::kWebGestureDeviceTouchscreen) {
        UMA_HISTOGRAM_ENUMERATION(kGestureHistogramName, i + 1,
                                  kMainThreadScrollingReasonEnumMax);
      } else {
        UMA_HISTOGRAM_ENUMERATION(kWheelHistogramName, i + 1,
                                  kMainThreadScrollingReasonEnumMax);
      }
    }
  }
}

void InputHandlerProxy::RecordScrollingThreadStatus(
    blink::WebGestureDevice device,
    uint32_t reasons) {
  if (device != blink::kWebGestureDeviceTouchpad &&
      device != blink::kWebGestureDeviceTouchscreen) {
    return;
  }

  ScrollingThreadStatus scrolling_thread_status = SCROLLING_ON_MAIN;
  if (reasons == cc::MainThreadScrollingReason::kNotScrollingOnMain) {
    int32_t event_disposition_result =
        (device == blink::kWebGestureDeviceTouchpad ? mouse_wheel_result_
                                                    : touch_result_);
    switch (event_disposition_result) {
      case kEventDispositionUndefined:
      case DID_NOT_HANDLE_NON_BLOCKING_DUE_TO_FLING:
      case DID_HANDLE_NON_BLOCKING:
      case DROP_EVENT:
        scrolling_thread_status = SCROLLING_ON_COMPOSITOR;
        break;
      case DID_NOT_HANDLE:
        scrolling_thread_status = SCROLLING_ON_COMPOSITOR_BLOCKED_ON_MAIN;
        break;
      default:
        NOTREACHED();
        scrolling_thread_status = SCROLLING_ON_COMPOSITOR;
    }
  }

  // UMA_HISTOGRAM_ENUMERATION requires that the enum_max must be strictly
  // greater than the sample value.
  const uint32_t kScrolingThreadStatusEnumMax =
      ScrollingThreadStatus::LAST_SCROLLING_THREAD_STATUS_VALUE + 1;

  if (device == blink::kWebGestureDeviceTouchscreen) {
    UMA_HISTOGRAM_ENUMERATION("Renderer4.GestureScrollingThreadStatus",
                              scrolling_thread_status,
                              kScrolingThreadStatusEnumMax);
  } else {
    UMA_HISTOGRAM_ENUMERATION("Renderer4.WheelScrollingThreadStatus",
                              scrolling_thread_status,
                              kScrolingThreadStatusEnumMax);
  }
}

bool InputHandlerProxy::ShouldAnimate(bool has_precise_scroll_deltas) const {
#if defined(OS_MACOSX)
  // Mac does not smooth scroll wheel events (crbug.com/574283).
  return false;
#else
  return smooth_scroll_enabled_ && !has_precise_scroll_deltas;
#endif
}

InputHandlerProxy::EventDisposition InputHandlerProxy::HandleMouseWheel(
    const WebMouseWheelEvent& wheel_event) {
  // Only call |CancelCurrentFling()| if a fling was active, as it will
  // otherwise disrupt an in-progress touch scroll.
  if (!wheel_event.has_precise_scrolling_deltas && fling_curve_)
    CancelCurrentFling();

  InputHandlerProxy::EventDisposition result = DROP_EVENT;

  if (wheel_event.dispatch_type == WebInputEvent::kEventNonBlocking) {
    // The first wheel event in the sequence should be cancellable.
    DCHECK(wheel_event.phase != WebMouseWheelEvent::kPhaseBegan);

    result = static_cast<EventDisposition>(mouse_wheel_result_);

    if (wheel_event.phase == WebMouseWheelEvent::kPhaseEnded ||
        wheel_event.phase == WebMouseWheelEvent::kPhaseCancelled ||
        wheel_event.momentum_phase == WebMouseWheelEvent::kPhaseEnded ||
        wheel_event.momentum_phase == WebMouseWheelEvent::kPhaseCancelled) {
      mouse_wheel_result_ = kEventDispositionUndefined;
    }
    if (mouse_wheel_result_ != kEventDispositionUndefined)
      return result;
  }

  cc::EventListenerProperties properties =
      input_handler_->GetEventListenerProperties(
          cc::EventListenerClass::kMouseWheel);
  switch (properties) {
    case cc::EventListenerProperties::kPassive:
      result = DID_HANDLE_NON_BLOCKING;
      break;
    case cc::EventListenerProperties::kBlockingAndPassive:
    case cc::EventListenerProperties::kBlocking:
      result = DID_NOT_HANDLE;
      break;
    case cc::EventListenerProperties::kNone:
      result = DROP_EVENT;
      break;
    default:
      NOTREACHED();
      result = DROP_EVENT;
  }

  mouse_wheel_result_ = result;
  return result;
}

InputHandlerProxy::EventDisposition InputHandlerProxy::HandleGestureScrollBegin(
    const WebGestureEvent& gesture_event) {
  TRACE_EVENT0("input", "InputHandlerProxy::HandleGestureScrollBegin");
  if (gesture_scroll_on_impl_thread_)
    CancelCurrentFling();

#ifndef NDEBUG
  expect_scroll_update_end_ = true;
#endif
  cc::ScrollState scroll_state = CreateScrollStateForGesture(gesture_event);
  cc::InputHandler::ScrollStatus scroll_status;
  if (gesture_event.data.scroll_begin.delta_hint_units ==
      blink::WebGestureEvent::ScrollUnits::kPage) {
    scroll_status.thread = cc::InputHandler::SCROLL_ON_MAIN_THREAD;
    scroll_status.main_thread_scrolling_reasons =
        cc::MainThreadScrollingReason::kContinuingMainThreadScroll;
  } else if (gesture_event.data.scroll_begin.target_viewport) {
    scroll_status = input_handler_->RootScrollBegin(
        &scroll_state, GestureScrollInputType(gesture_event.SourceDevice()));
  } else if (ShouldAnimate(gesture_event.data.scroll_begin.delta_hint_units !=
                           blink::WebGestureEvent::ScrollUnits::kPixels)) {
    DCHECK(!scroll_state.is_in_inertial_phase());
    scroll_status = input_handler_->ScrollAnimatedBegin(&scroll_state);
  } else {
    scroll_status = input_handler_->ScrollBegin(
        &scroll_state, GestureScrollInputType(gesture_event.SourceDevice()));
  }
  RecordMainThreadScrollingReasons(gesture_event.SourceDevice(),
                                   scroll_status.main_thread_scrolling_reasons);

  RecordScrollingThreadStatus(gesture_event.SourceDevice(),
                              scroll_status.main_thread_scrolling_reasons);

  InputHandlerProxy::EventDisposition result = DID_NOT_HANDLE;
  scroll_sequence_ignored_ = false;
  in_inertial_scrolling_ = false;
  switch (scroll_status.thread) {
    case cc::InputHandler::SCROLL_ON_IMPL_THREAD:
      TRACE_EVENT_INSTANT0("input", "Handle On Impl", TRACE_EVENT_SCOPE_THREAD);
      gesture_scroll_on_impl_thread_ = true;
      if (input_handler_->IsCurrentlyScrollingViewport())
        client_->DidStartScrollingViewport();

      if (scroll_status.bubble)
        result = DID_HANDLE_SHOULD_BUBBLE;
      else
        result = DID_HANDLE;
      break;
    case cc::InputHandler::SCROLL_UNKNOWN:
    case cc::InputHandler::SCROLL_ON_MAIN_THREAD:
      TRACE_EVENT_INSTANT0("input", "Handle On Main", TRACE_EVENT_SCOPE_THREAD);
      result = DID_NOT_HANDLE;
      break;
    case cc::InputHandler::SCROLL_IGNORED:
      TRACE_EVENT_INSTANT0("input", "Ignore Scroll", TRACE_EVENT_SCOPE_THREAD);
      scroll_sequence_ignored_ = true;
      result = DROP_EVENT;
      break;
  }
  if (scroll_elasticity_controller_ && result != DID_NOT_HANDLE)
    HandleScrollElasticityOverscroll(gesture_event,
                                     cc::InputHandlerScrollResult());

  return result;
}

InputHandlerProxy::EventDisposition
InputHandlerProxy::HandleGestureScrollUpdate(
    const WebGestureEvent& gesture_event) {
#ifndef NDEBUG
  DCHECK(expect_scroll_update_end_);
#endif

  gfx::Vector2dF scroll_delta(-gesture_event.data.scroll_update.delta_x,
                              -gesture_event.data.scroll_update.delta_y);
  TRACE_EVENT2("input", "InputHandlerProxy::HandleGestureScrollUpdate", "dx",
               scroll_delta.x(), "dy", scroll_delta.y());

  if (scroll_sequence_ignored_) {
    TRACE_EVENT_INSTANT0("input", "Scroll Sequence Ignored",
                         TRACE_EVENT_SCOPE_THREAD);
    return DROP_EVENT;
  }

  if (!gesture_scroll_on_impl_thread_ && !gesture_pinch_on_impl_thread_)
    return DID_NOT_HANDLE;

  cc::ScrollState scroll_state = CreateScrollStateForGesture(gesture_event);
  in_inertial_scrolling_ = scroll_state.is_in_inertial_phase();
  gfx::PointF scroll_point(gesture_event.PositionInWidget());

  if (ShouldAnimate(gesture_event.data.scroll_update.delta_units !=
                    blink::WebGestureEvent::ScrollUnits::kPixels)) {
    DCHECK(!scroll_state.is_in_inertial_phase());
    base::TimeTicks event_time = gesture_event.TimeStamp();
    base::TimeDelta delay = base::TimeTicks::Now() - event_time;
    switch (input_handler_
                ->ScrollAnimated(gfx::ToFlooredPoint(scroll_point),
                                 scroll_delta, delay)
                .thread) {
      case cc::InputHandler::SCROLL_ON_IMPL_THREAD:
        return DID_HANDLE;
      case cc::InputHandler::SCROLL_IGNORED:
        TRACE_EVENT_INSTANT0("input", "Scroll Ignored",
                             TRACE_EVENT_SCOPE_THREAD);
        return DROP_EVENT;
      case cc::InputHandler::SCROLL_ON_MAIN_THREAD:
      case cc::InputHandler::SCROLL_UNKNOWN:
        if (input_handler_->ScrollingShouldSwitchtoMainThread()) {
          TRACE_EVENT_INSTANT0("input", "Move Scroll To Main Thread",
                               TRACE_EVENT_SCOPE_THREAD);
          gesture_scroll_on_impl_thread_ = false;
          client_->GenerateScrollBeginAndSendToMainThread(gesture_event);
        }
        return DID_NOT_HANDLE;
    }
  }

  if (snap_fling_controller_->HandleGestureScrollUpdate(
          GetGestureScrollUpdateInfo(gesture_event))) {
#ifndef NDEBUG
    expect_scroll_update_end_ = false;
#endif
    gesture_scroll_on_impl_thread_ = false;
    return DROP_EVENT;
  }

  cc::InputHandlerScrollResult scroll_result =
      input_handler_->ScrollBy(&scroll_state);

  if (!scroll_result.did_scroll &&
      input_handler_->ScrollingShouldSwitchtoMainThread() &&
      ((gesture_event.SourceDevice() == blink::kWebGestureDeviceTouchpad &&
        touchpad_and_wheel_scroll_latching_enabled_) ||
       gesture_event.SourceDevice() == blink::kWebGestureDeviceTouchscreen)) {
    gesture_scroll_on_impl_thread_ = false;
    client_->GenerateScrollBeginAndSendToMainThread(gesture_event);

    if (!gesture_pinch_on_impl_thread_)
      return DID_NOT_HANDLE;
  }

  HandleOverscroll(scroll_point, scroll_result, true);

  if (scroll_elasticity_controller_)
    HandleScrollElasticityOverscroll(gesture_event, scroll_result);

  return scroll_result.did_scroll ? DID_HANDLE : DROP_EVENT;
}

InputHandlerProxy::EventDisposition InputHandlerProxy::HandleGestureScrollEnd(
  const WebGestureEvent& gesture_event) {
  TRACE_EVENT0("input", "InputHandlerProxy::HandleGestureScrollEnd");
#ifndef NDEBUG
  DCHECK(expect_scroll_update_end_);
  expect_scroll_update_end_ = false;
#endif
  if (ShouldAnimate(gesture_event.data.scroll_end.delta_units !=
                    blink::WebGestureEvent::ScrollUnits::kPixels)) {
    // Do nothing if the scroll is being animated; the scroll animation will
    // generate the ScrollEnd when it is done.
  } else {
    cc::ScrollState scroll_state = CreateScrollStateForGesture(gesture_event);
    input_handler_->ScrollEnd(&scroll_state, true);
  }

  if (scroll_sequence_ignored_)
    return DROP_EVENT;

  if (!gesture_scroll_on_impl_thread_)
    return DID_NOT_HANDLE;

  if (scroll_elasticity_controller_)
    HandleScrollElasticityOverscroll(gesture_event,
                                     cc::InputHandlerScrollResult());

  gesture_scroll_on_impl_thread_ = false;
  return DID_HANDLE;
}

InputHandlerProxy::EventDisposition InputHandlerProxy::HandleGestureFlingStart(
    const WebGestureEvent& gesture_event) {
  // Touchpad and touchscreen flings are handled on browser.
  DCHECK_NE(blink::kWebGestureDeviceTouchpad, gesture_event.SourceDevice());
  DCHECK_NE(blink::kWebGestureDeviceTouchscreen, gesture_event.SourceDevice());
#ifndef NDEBUG
  expect_scroll_update_end_ = false;
#endif

  if (scroll_sequence_ignored_)
    return DROP_EVENT;

  cc::ScrollState scroll_state = CreateScrollStateForGesture(gesture_event);
  cc::InputHandler::ScrollStatus scroll_status;
  scroll_status.main_thread_scrolling_reasons =
      cc::MainThreadScrollingReason::kNotScrollingOnMain;
  switch (gesture_event.SourceDevice()) {
    case blink::kWebGestureDeviceSyntheticAutoscroll:
      if (!gesture_scroll_on_impl_thread_) {
        scroll_status.thread = cc::InputHandler::SCROLL_ON_MAIN_THREAD;
        scroll_status.main_thread_scrolling_reasons =
            cc::MainThreadScrollingReason::kContinuingMainThreadScroll;
      } else {
        scroll_status = input_handler_->FlingScrollBegin();
      }
      break;
    case blink::kWebGestureDeviceTouchscreen:
    case blink::kWebGestureDeviceTouchpad:
    case blink::kWebGestureDeviceUninitialized:
    case blink::kWebGestureDeviceCount:
      NOTREACHED();
      return DID_NOT_HANDLE;
  }

  switch (scroll_status.thread) {
    case cc::InputHandler::SCROLL_ON_IMPL_THREAD: {
      const float vx = gesture_event.data.fling_start.velocity_x;
      const float vy = gesture_event.data.fling_start.velocity_y;
      UpdateCurrentFlingState(gesture_event, gfx::Vector2dF(vx, vy));
      TRACE_EVENT_ASYNC_BEGIN2("input,benchmark,rail",
                               "InputHandlerProxy::HandleGestureFling::started",
                               this, "vx", vx, "vy", vy);
      // Note that the timestamp will only be used to kickstart the animation if
      // its sufficiently close to the timestamp of the first call |Animate()|.
      has_fling_animation_started_ = false;
      fling_booster_ = std::make_unique<FlingBooster>(
          current_fling_velocity_, fling_parameters_.source_device,
          fling_parameters_.modifiers);
      RequestAnimation();
      return DID_HANDLE;
    }
    case cc::InputHandler::SCROLL_UNKNOWN:
    case cc::InputHandler::SCROLL_ON_MAIN_THREAD: {
      TRACE_EVENT_INSTANT0("input,rail",
                           "InputHandlerProxy::HandleGestureFling::"
                           "scroll_on_main_thread",
                           TRACE_EVENT_SCOPE_THREAD);
      gesture_scroll_on_impl_thread_ = false;
      fling_may_be_active_on_main_thread_ = true;
      return DID_NOT_HANDLE;
    }
    case cc::InputHandler::SCROLL_IGNORED: {
      TRACE_EVENT_INSTANT0(
          "input,rail",
          "InputHandlerProxy::HandleGestureFling::ignored",
          TRACE_EVENT_SCOPE_THREAD);
      gesture_scroll_on_impl_thread_ = false;
      return DROP_EVENT;
    }
  }
  return DID_NOT_HANDLE;
}

InputHandlerProxy::EventDisposition InputHandlerProxy::HitTestTouchEvent(
    const blink::WebTouchEvent& touch_event,
    bool* is_touching_scrolling_layer,
    cc::TouchAction* white_listed_touch_action) {
  *is_touching_scrolling_layer = false;
  EventDisposition result = DROP_EVENT;
  for (size_t i = 0; i < touch_event.touches_length; ++i) {
    if (touch_event.touch_start_or_first_touch_move)
      DCHECK(white_listed_touch_action);
    else
      DCHECK(!white_listed_touch_action);

    if (touch_event.GetType() == WebInputEvent::kTouchStart &&
        touch_event.touches[i].state != WebTouchPoint::kStatePressed) {
      continue;
    }

    cc::TouchAction touch_action = cc::kTouchActionAuto;
    cc::InputHandler::TouchStartOrMoveEventListenerType event_listener_type =
        input_handler_->EventListenerTypeForTouchStartOrMoveAt(
            gfx::Point(touch_event.touches[i].PositionInWidget().x,
                       touch_event.touches[i].PositionInWidget().y),
            &touch_action);
    if (white_listed_touch_action)
      *white_listed_touch_action &= touch_action;

    if (event_listener_type !=
        cc::InputHandler::TouchStartOrMoveEventListenerType::NO_HANDLER) {
      *is_touching_scrolling_layer =
          event_listener_type ==
          cc::InputHandler::TouchStartOrMoveEventListenerType::
              HANDLER_ON_SCROLLING_LAYER;
      result = DID_NOT_HANDLE;
      break;
    }
  }

  // If |result| is DROP_EVENT it wasn't processed above.
  if (result == DROP_EVENT) {
    switch (input_handler_->GetEventListenerProperties(
        cc::EventListenerClass::kTouchStartOrMove)) {
      case cc::EventListenerProperties::kPassive:
        result = DID_HANDLE_NON_BLOCKING;
        break;
      case cc::EventListenerProperties::kBlocking:
        // The touch area rects above already have checked whether it hits
        // a blocking region. Since it does not the event can be dropped.
        result = DROP_EVENT;
        break;
      case cc::EventListenerProperties::kBlockingAndPassive:
        // There is at least one passive listener that needs to possibly
        // be notified so it can't be dropped.
        result = DID_HANDLE_NON_BLOCKING;
        break;
      case cc::EventListenerProperties::kNone:
        result = DROP_EVENT;
        break;
      default:
        NOTREACHED();
        result = DROP_EVENT;
        break;
    }
  }

  // Merge |touch_result_| and |result| so the result has the highest
  // priority value according to the sequence; (DROP_EVENT,
  // DID_HANDLE_NON_BLOCKING, DID_NOT_HANDLE).
  if (touch_result_ == kEventDispositionUndefined ||
      touch_result_ == DROP_EVENT || result == DID_NOT_HANDLE)
    touch_result_ = result;
  return result;
}

InputHandlerProxy::EventDisposition InputHandlerProxy::HandleTouchStart(
    const blink::WebTouchEvent& touch_event) {
  bool is_touching_scrolling_layer;
  cc::TouchAction white_listed_touch_action = cc::kTouchActionAuto;
  EventDisposition result = HitTestTouchEvent(
      touch_event, &is_touching_scrolling_layer, &white_listed_touch_action);

  // If |result| is still DROP_EVENT look at the touch end handler as
  // we may not want to discard the entire touch sequence. Note this
  // code is explicitly after the assignment of the |touch_result_|
  // so the touch moves are not sent to the main thread un-necessarily.
  if (result == DROP_EVENT &&
      input_handler_->GetEventListenerProperties(
          cc::EventListenerClass::kTouchEndOrCancel) !=
          cc::EventListenerProperties::kNone) {
    result = DID_HANDLE_NON_BLOCKING;
  }

  bool is_in_inertial_scrolling_on_impl =
      in_inertial_scrolling_ && gesture_scroll_on_impl_thread_;
  if (is_in_inertial_scrolling_on_impl && is_touching_scrolling_layer)
    result = DID_NOT_HANDLE_NON_BLOCKING_DUE_TO_FLING;

  client_->SetWhiteListedTouchAction(white_listed_touch_action,
                                     touch_event.unique_touch_event_id, result);

  return result;
}

InputHandlerProxy::EventDisposition InputHandlerProxy::HandleTouchMove(
    const blink::WebTouchEvent& touch_event) {
  // Hit test if this is the first touch move or we don't have any results
  // from a previous hit test.
  if (touch_result_ == kEventDispositionUndefined ||
      touch_event.touch_start_or_first_touch_move) {
    bool is_touching_scrolling_layer;
    cc::TouchAction white_listed_touch_action = cc::kTouchActionAuto;
    EventDisposition result = HitTestTouchEvent(
        touch_event, &is_touching_scrolling_layer, &white_listed_touch_action);
    client_->SetWhiteListedTouchAction(
        white_listed_touch_action, touch_event.unique_touch_event_id, result);
    return result;
  }
  return static_cast<EventDisposition>(touch_result_);
}

InputHandlerProxy::EventDisposition InputHandlerProxy::HandleTouchEnd(
    const blink::WebTouchEvent& touch_event) {
  if (touch_event.touches_length == 1)
    touch_result_ = kEventDispositionUndefined;
  return DID_NOT_HANDLE;
}

void InputHandlerProxy::Animate(base::TimeTicks time) {
  // If using synchronous animate, then only expect Animate attempts started by
  // the synchronous system. Don't let the InputHandler try to Animate also.
  DCHECK(!input_handler_->IsCurrentlyScrollingViewport() ||
         allow_root_animate_);

  if (scroll_elasticity_controller_)
    scroll_elasticity_controller_->Animate(time);

  snap_fling_controller_->Animate(time);

  if (!fling_curve_)
    return;

  double monotonic_time_sec = InSecondsF(time);
  DCHECK(fling_booster_);
  fling_booster_->set_last_fling_animation_time(monotonic_time_sec);
  if (fling_booster_->MustCancelDeferredFling()) {
    CancelCurrentFling();
    return;
  }

  client_->DidAnimateForInput();

  if (!has_fling_animation_started_) {
    has_fling_animation_started_ = true;
    // Guard against invalid, future or sufficiently stale start times, as there
    // are no guarantees fling event and animation timestamps are compatible.
    if (fling_parameters_.start_time.is_null() ||
        time <= fling_parameters_.start_time ||
        time >= fling_parameters_.start_time +
                    kMaxIntervalFromFlingTimestampToFirstAnimate) {
      fling_parameters_.start_time = time;
      RequestAnimation();
      return;
    }
  }

  bool fling_is_active = fling_curve_->AdvanceAndApplyToTarget(
      (time - fling_parameters_.start_time).InSecondsF(), this);

  if (disallow_vertical_fling_scroll_ && disallow_horizontal_fling_scroll_)
    fling_is_active = false;

  if (fling_is_active) {
    RequestAnimation();
  } else {
    TRACE_EVENT_INSTANT0("input",
                         "InputHandlerProxy::animate::flingOver",
                         TRACE_EVENT_SCOPE_THREAD);
    if (fling_parameters_.source_device !=
        blink::kWebGestureDeviceSyntheticAutoscroll)
      CancelCurrentFling();
  }
}

void InputHandlerProxy::MainThreadHasStoppedFlinging() {
  fling_may_be_active_on_main_thread_ = false;
  client_->DidStopFlinging();
}

void InputHandlerProxy::ReconcileElasticOverscrollAndRootScroll() {
  if (scroll_elasticity_controller_)
    scroll_elasticity_controller_->ReconcileStretchAndScroll();
}

void InputHandlerProxy::UpdateRootLayerStateForSynchronousInputHandler(
    const gfx::ScrollOffset& total_scroll_offset,
    const gfx::ScrollOffset& max_scroll_offset,
    const gfx::SizeF& scrollable_size,
    float page_scale_factor,
    float min_page_scale_factor,
    float max_page_scale_factor) {
  if (synchronous_input_handler_) {
    synchronous_input_handler_->UpdateRootLayerState(
        total_scroll_offset, max_scroll_offset, scrollable_size,
        page_scale_factor, min_page_scale_factor, max_page_scale_factor);
  }
}

void InputHandlerProxy::DeliverInputForBeginFrame() {
  DispatchQueuedInputEvents();
}

void InputHandlerProxy::SetOnlySynchronouslyAnimateRootFlings(
    SynchronousInputHandler* synchronous_input_handler) {
  allow_root_animate_ = !synchronous_input_handler;
  synchronous_input_handler_ = synchronous_input_handler;
  if (synchronous_input_handler_)
    input_handler_->RequestUpdateForSynchronousInputHandler();
}

void InputHandlerProxy::SynchronouslyAnimate(base::TimeTicks time) {
  // When this function is used, SetOnlySynchronouslyAnimate() should have been
  // previously called. IOW you should either be entirely in synchronous mode or
  // not.
  DCHECK(synchronous_input_handler_);
  DCHECK(!allow_root_animate_);
  base::AutoReset<bool> reset(&allow_root_animate_, true);
  Animate(time);
}

void InputHandlerProxy::SynchronouslySetRootScrollOffset(
    const gfx::ScrollOffset& root_offset) {
  DCHECK(synchronous_input_handler_);
  input_handler_->SetSynchronousInputHandlerRootScrollOffset(root_offset);
}

void InputHandlerProxy::SynchronouslyZoomBy(float magnify_delta,
                                            const gfx::Point& anchor) {
  DCHECK(synchronous_input_handler_);
  input_handler_->PinchGestureBegin();
  input_handler_->PinchGestureUpdate(magnify_delta, anchor);
  input_handler_->PinchGestureEnd(anchor, false);
}

bool InputHandlerProxy::GetSnapFlingInfo(
    const gfx::Vector2dF& natural_displacement,
    gfx::Vector2dF* initial_offset,
    gfx::Vector2dF* target_offset) const {
  return input_handler_->GetSnapFlingInfo(natural_displacement, initial_offset,
                                          target_offset);
}

gfx::Vector2dF InputHandlerProxy::ScrollByForSnapFling(
    const gfx::Vector2dF& delta) {
  cc::ScrollState scroll_state = CreateScrollStateForInertialUpdate(delta);
  cc::InputHandlerScrollResult scroll_result =
      input_handler_->ScrollBy(&scroll_state);
  return scroll_result.current_visual_offset;
}

void InputHandlerProxy::ScrollEndForSnapFling() {
  cc::ScrollState scroll_state = CreateScrollStateForInertialEnd();
  input_handler_->ScrollEnd(&scroll_state, false);
}

void InputHandlerProxy::RequestAnimationForSnapFling() {
  RequestAnimation();
}

void InputHandlerProxy::HandleOverscroll(
    const gfx::PointF& causal_event_viewport_point,
    const cc::InputHandlerScrollResult& scroll_result,
    bool bundle_overscroll_params_with_ack) {
  DCHECK(client_);
  if (!scroll_result.did_overscroll_root)
    return;

  TRACE_EVENT2("input",
               "InputHandlerProxy::DidOverscroll",
               "dx",
               scroll_result.unused_scroll_delta.x(),
               "dy",
               scroll_result.unused_scroll_delta.y());

  if (fling_curve_) {
    static const int kFlingOverscrollThreshold = 1;
    disallow_horizontal_fling_scroll_ |=
        std::abs(scroll_result.accumulated_root_overscroll.x()) >=
        kFlingOverscrollThreshold;
    disallow_vertical_fling_scroll_ |=
        std::abs(scroll_result.accumulated_root_overscroll.y()) >=
        kFlingOverscrollThreshold;
  }

  if (bundle_overscroll_params_with_ack) {
    // Bundle overscroll message with triggering event response, saving an IPC.
    current_overscroll_params_.reset(new DidOverscrollParams());
    current_overscroll_params_->accumulated_overscroll =
        scroll_result.accumulated_root_overscroll;
    current_overscroll_params_->latest_overscroll_delta =
        scroll_result.unused_scroll_delta;
    current_overscroll_params_->current_fling_velocity =
        ToClientScrollIncrement(current_fling_velocity_);
    current_overscroll_params_->causal_event_viewport_point =
        causal_event_viewport_point;
    current_overscroll_params_->overscroll_behavior =
        scroll_result.overscroll_behavior;
    return;
  }

  client_->DidOverscroll(scroll_result.accumulated_root_overscroll,
                         scroll_result.unused_scroll_delta,
                         ToClientScrollIncrement(current_fling_velocity_),
                         causal_event_viewport_point,
                         scroll_result.overscroll_behavior);
}

bool InputHandlerProxy::CancelCurrentFling() {
  if (CancelCurrentFlingWithoutNotifyingClient()) {
    client_->DidStopFlinging();
    return true;
  }
  return false;
}

bool InputHandlerProxy::CancelCurrentFlingWithoutNotifyingClient() {
  bool had_fling_animation = !!fling_curve_;
  if (had_fling_animation &&
      fling_parameters_.source_device == blink::kWebGestureDeviceTouchscreen) {
    cc::ScrollStateData scroll_state_data;
    scroll_state_data.is_ending = true;
    cc::ScrollState scroll_state(scroll_state_data);
    input_handler_->ScrollEnd(&scroll_state, false);
    TRACE_EVENT_ASYNC_END0(
        "input",
        "InputHandlerProxy::HandleGestureFling::started",
        this);
  }

  TRACE_EVENT_INSTANT1("input",
                       "InputHandlerProxy::CancelCurrentFling",
                       TRACE_EVENT_SCOPE_THREAD,
                       "had_fling_animation",
                       had_fling_animation);
  fling_curve_.reset();
  has_fling_animation_started_ = false;
  gesture_scroll_on_impl_thread_ = false;
  current_fling_velocity_ = gfx::Vector2dF();
  fling_parameters_ = blink::WebActiveFlingParameters();

  if (fling_booster_) {
    if (fling_booster_->fling_cancellation_is_deferred()) {
      WebGestureEvent last_fling_boost_event =
          fling_booster_->last_boost_event();
      fling_booster_.reset();
      if (last_fling_boost_event.GetType() ==
              WebInputEvent::kGestureScrollBegin ||
          last_fling_boost_event.GetType() ==
              WebInputEvent::kGestureScrollUpdate) {
        // Synthesize a GestureScrollBegin, as the original was suppressed.
        HandleInputEvent(ObtainGestureScrollBegin(last_fling_boost_event));
      }
    } else {
      fling_booster_.reset();
    }
  }

  return had_fling_animation;
}

void InputHandlerProxy::RequestAnimation() {
  // When a SynchronousInputHandler is present, root flings should go through
  // it to allow it to control when or if the root fling is animated. Non-root
  // flings always go through the normal InputHandler.
  if (synchronous_input_handler_ &&
      input_handler_->IsCurrentlyScrollingViewport())
    synchronous_input_handler_->SetNeedsSynchronousAnimateInput();
  else
    input_handler_->SetNeedsAnimateInput();
}

bool InputHandlerProxy::ScrollBy(const WebFloatSize& increment,
                                 const WebFloatSize& velocity) {
  WebFloatSize clipped_increment;
  WebFloatSize clipped_velocity;
  if (!disallow_horizontal_fling_scroll_) {
    clipped_increment.width = increment.width;
    clipped_velocity.width = velocity.width;
  }
  if (!disallow_vertical_fling_scroll_) {
    clipped_increment.height = increment.height;
    clipped_velocity.height = velocity.height;
  }

  current_fling_velocity_ = clipped_velocity;
  DCHECK(fling_booster_);
  if (fling_booster_)
    fling_booster_->set_current_fling_velocity(current_fling_velocity_);

  // Early out if the increment is zero, but avoid early termination if the
  // velocity is still non-zero.
  if (clipped_increment == WebFloatSize())
    return clipped_velocity != WebFloatSize();

  TRACE_EVENT2("input",
               "InputHandlerProxy::scrollBy",
               "x",
               clipped_increment.width,
               "y",
               clipped_increment.height);

  bool did_scroll = false;

  switch (fling_parameters_.source_device) {
    case blink::kWebGestureDeviceSyntheticAutoscroll: {
      clipped_increment = ToClientScrollIncrement(clipped_increment);
      cc::ScrollStateData scroll_state_data;
      scroll_state_data.delta_x = clipped_increment.width;
      scroll_state_data.delta_y = clipped_increment.height;
      scroll_state_data.velocity_x = clipped_velocity.width;
      scroll_state_data.velocity_y = clipped_velocity.height;
      scroll_state_data.is_in_inertial_phase = true;
      cc::ScrollState scroll_state(scroll_state_data);
      cc::InputHandlerScrollResult scroll_result =
          input_handler_->ScrollBy(&scroll_state);
      HandleOverscroll(fling_parameters_.point, scroll_result, false);
      did_scroll = scroll_result.did_scroll;
    } break;
    case blink::kWebGestureDeviceTouchscreen:
    case blink::kWebGestureDeviceTouchpad:
    case blink::kWebGestureDeviceUninitialized:
    case blink::kWebGestureDeviceCount:
      NOTREACHED();
      return false;
  }

  if (did_scroll) {
    fling_parameters_.cumulative_scroll.width += clipped_increment.width;
    fling_parameters_.cumulative_scroll.height += clipped_increment.height;
  }

  // It's possible the provided |increment| is sufficiently small as to not
  // trigger a scroll, e.g., with a trivial time delta between fling updates.
  // Return true in this case to prevent early fling termination.
  if (std::abs(clipped_increment.width) < kScrollEpsilon &&
      std::abs(clipped_increment.height) < kScrollEpsilon)
    return true;

  return did_scroll;
}

void InputHandlerProxy::HandleScrollElasticityOverscroll(
    const WebGestureEvent& gesture_event,
    const cc::InputHandlerScrollResult& scroll_result) {
  DCHECK(scroll_elasticity_controller_);
  // Send the event and its disposition to the elasticity controller to update
  // the over-scroll animation. Note that the call to the elasticity controller
  // is made asynchronously, to minimize divergence between main thread and
  // impl thread event handling paths.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &InputScrollElasticityController::ObserveGestureEventAndResult,
          scroll_elasticity_controller_->GetWeakPtr(), gesture_event,
          scroll_result));
}

void InputHandlerProxy::SetTickClockForTesting(
    const base::TickClock* tick_clock) {
  tick_clock_ = tick_clock;
}

void InputHandlerProxy::UpdateCurrentFlingState(
    const WebGestureEvent& fling_start_event,
    const gfx::Vector2dF& velocity) {
  DCHECK_EQ(WebInputEvent::kGestureFlingStart, fling_start_event.GetType());
  // Flings with touchpad and touchscreen source devices are handled on browser.
  DCHECK(fling_start_event.SourceDevice() != blink::kWebGestureDeviceTouchpad &&
         fling_start_event.SourceDevice() !=
             blink::kWebGestureDeviceTouchscreen);
  current_fling_velocity_ = velocity;
  fling_curve_ = client_->CreateFlingAnimationCurve(
      fling_start_event.SourceDevice(),
      WebFloatPoint(velocity.x(), velocity.y()), blink::WebSize());
  disallow_horizontal_fling_scroll_ = !velocity.x();
  disallow_vertical_fling_scroll_ = !velocity.y();
  fling_parameters_.start_time = fling_start_event.TimeStamp();
  fling_parameters_.delta = WebFloatPoint(velocity.x(), velocity.y());
  fling_parameters_.point = fling_start_event.PositionInWidget();
  fling_parameters_.global_point = fling_start_event.PositionInScreen();
  fling_parameters_.modifiers = fling_start_event.GetModifiers();
  fling_parameters_.source_device = fling_start_event.SourceDevice();
}

}  // namespace ui
