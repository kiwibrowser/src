// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/mouse_wheel_event_queue.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/location.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/renderer_host/input/timeout_monitor.h"
#include "content/common/input/synthetic_web_input_event_builders.h"
#include "content/public/common/content_features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "ui/events/base_event_utils.h"

using blink::WebGestureEvent;
using blink::WebInputEvent;
using blink::WebMouseWheelEvent;

namespace content {
namespace {

const float kWheelScrollX = 10;
const float kWheelScrollY = 12;
const float kWheelScrollGlobalX = 50;
const float kWheelScrollGlobalY = 72;

#define EXPECT_GESTURE_SCROLL_BEGIN_IMPL(event)                    \
  EXPECT_EQ(WebInputEvent::kGestureScrollBegin, event->GetType()); \
  EXPECT_EQ(kWheelScrollX, event->PositionInWidget().x);           \
  EXPECT_EQ(kWheelScrollY, event->PositionInWidget().y);           \
  EXPECT_EQ(kWheelScrollGlobalX, event->PositionInScreen().x);     \
  EXPECT_EQ(kWheelScrollGlobalY, event->PositionInScreen().y);     \
  EXPECT_EQ(scroll_units, event->data.scroll_begin.delta_hint_units);

#define EXPECT_GESTURE_SCROLL_BEGIN(event)          \
  EXPECT_GESTURE_SCROLL_BEGIN_IMPL(event);          \
  EXPECT_FALSE(event->data.scroll_begin.synthetic); \
  EXPECT_EQ(WebGestureEvent::kUnknownMomentumPhase, \
            event->data.scroll_begin.inertial_phase);

#define EXPECT_GESTURE_SCROLL_BEGIN_WITH_PHASE(event) \
  EXPECT_GESTURE_SCROLL_BEGIN_IMPL(event);            \
  EXPECT_FALSE(event->data.scroll_begin.synthetic);   \
  EXPECT_EQ(WebGestureEvent::kNonMomentumPhase,       \
            event->data.scroll_begin.inertial_phase);

#define EXPECT_SYNTHETIC_GESTURE_SCROLL_BEGIN(event) \
  EXPECT_GESTURE_SCROLL_BEGIN_IMPL(event);           \
  EXPECT_TRUE(event->data.scroll_begin.synthetic);   \
  EXPECT_EQ(WebGestureEvent::kNonMomentumPhase,      \
            event->data.scroll_begin.inertial_phase);

#define EXPECT_INERTIAL_GESTURE_SCROLL_BEGIN(event) \
  EXPECT_GESTURE_SCROLL_BEGIN_IMPL(event);          \
  EXPECT_FALSE(event->data.scroll_begin.synthetic); \
  EXPECT_EQ(WebGestureEvent::kMomentumPhase,        \
            event->data.scroll_begin.inertial_phase);

#define EXPECT_SYNTHETIC_INERTIAL_GESTURE_SCROLL_BEGIN(event) \
  EXPECT_GESTURE_SCROLL_BEGIN_IMPL(event);                    \
  EXPECT_TRUE(event->data.scroll_begin.synthetic);            \
  EXPECT_EQ(WebGestureEvent::kMomentumPhase,                  \
            event->data.scroll_begin.inertial_phase);

#define EXPECT_GESTURE_SCROLL_UPDATE_IMPL(event)                    \
  EXPECT_EQ(WebInputEvent::kGestureScrollUpdate, event->GetType()); \
  EXPECT_EQ(scroll_units, event->data.scroll_update.delta_units);   \
  EXPECT_EQ(kWheelScrollX, event->PositionInWidget().x);            \
  EXPECT_EQ(kWheelScrollY, event->PositionInWidget().y);            \
  EXPECT_EQ(kWheelScrollGlobalX, event->PositionInScreen().x);      \
  EXPECT_EQ(kWheelScrollGlobalY, event->PositionInScreen().y);

#define EXPECT_GESTURE_SCROLL_UPDATE(event)         \
  EXPECT_GESTURE_SCROLL_UPDATE_IMPL(event);         \
  EXPECT_EQ(WebGestureEvent::kUnknownMomentumPhase, \
            event->data.scroll_update.inertial_phase);

#define EXPECT_GESTURE_SCROLL_UPDATE_WITH_PHASE(event) \
  EXPECT_GESTURE_SCROLL_UPDATE_IMPL(event);            \
  EXPECT_EQ(WebGestureEvent::kNonMomentumPhase,        \
            event->data.scroll_update.inertial_phase);

#define EXPECT_INERTIAL_GESTURE_SCROLL_UPDATE(event) \
  EXPECT_GESTURE_SCROLL_UPDATE_IMPL(event);          \
  EXPECT_EQ(WebGestureEvent::kMomentumPhase,         \
            event->data.scroll_update.inertial_phase);

#define EXPECT_GESTURE_SCROLL_END_IMPL(event)                    \
  EXPECT_EQ(WebInputEvent::kGestureScrollEnd, event->GetType()); \
  EXPECT_EQ(scroll_units, event->data.scroll_end.delta_units);   \
  EXPECT_EQ(kWheelScrollX, event->PositionInWidget().x);         \
  EXPECT_EQ(kWheelScrollY, event->PositionInWidget().y);         \
  EXPECT_EQ(kWheelScrollGlobalX, event->PositionInScreen().x);   \
  EXPECT_EQ(kWheelScrollGlobalY, event->PositionInScreen().y);

#define EXPECT_GESTURE_SCROLL_END(event)            \
  EXPECT_GESTURE_SCROLL_END_IMPL(event);            \
  EXPECT_FALSE(event->data.scroll_end.synthetic);   \
  EXPECT_EQ(WebGestureEvent::kUnknownMomentumPhase, \
            event->data.scroll_end.inertial_phase);

#define EXPECT_GESTURE_SCROLL_END_WITH_PHASE(event) \
  EXPECT_GESTURE_SCROLL_END_IMPL(event);            \
  EXPECT_FALSE(event->data.scroll_end.synthetic);   \
  EXPECT_EQ(WebGestureEvent::kNonMomentumPhase,     \
            event->data.scroll_end.inertial_phase);

#define EXPECT_SYNTHETIC_GESTURE_SCROLL_END(event) \
  EXPECT_GESTURE_SCROLL_END_IMPL(event);           \
  EXPECT_TRUE(event->data.scroll_end.synthetic);   \
  EXPECT_EQ(WebGestureEvent::kNonMomentumPhase,    \
            event->data.scroll_end.inertial_phase);

#define EXPECT_INERTIAL_GESTURE_SCROLL_END(event) \
  EXPECT_GESTURE_SCROLL_END_IMPL(event);          \
  EXPECT_FALSE(event->data.scroll_end.synthetic); \
  EXPECT_EQ(WebGestureEvent::kMomentumPhase,      \
            event->data.scroll_end.inertial_phase);

#define EXPECT_SYNTHETIC_INERTIAL_GESTURE_SCROLL_END(event) \
  EXPECT_GESTURE_SCROLL_END_IMPL(event);                    \
  EXPECT_TRUE(event->data.scroll_end.synthetic);            \
  EXPECT_EQ(WebGestureEvent::kMomentumPhase,                \
            event->data.scroll_end.inertial_phase);

#define EXPECT_MOUSE_WHEEL(event) \
  EXPECT_EQ(WebInputEvent::kMouseWheel, event->GetType());

enum WheelScrollingMode {
  kWheelScrollingModeNone,
  kWheelScrollLatching,
  kAsyncWheelEvents,
};

}  // namespace

class MouseWheelEventQueueTest
    : public testing::TestWithParam<WheelScrollingMode>,
      public MouseWheelEventQueueClient {
 public:
  MouseWheelEventQueueTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI),
        acked_event_count_(0),
        last_acked_event_state_(INPUT_EVENT_ACK_STATE_UNKNOWN) {
    scroll_latching_enabled_ = GetParam() != kWheelScrollingModeNone;
    switch (GetParam()) {
      case kWheelScrollingModeNone:
        feature_list_.InitWithFeatures(
            {}, {features::kTouchpadAndWheelScrollLatching,
                 features::kAsyncWheelEvents});
        break;
      case kWheelScrollLatching:
        feature_list_.InitWithFeatures(
            {features::kTouchpadAndWheelScrollLatching},
            {features::kAsyncWheelEvents});
        break;
      case kAsyncWheelEvents:
        feature_list_.InitWithFeatures(
            {features::kTouchpadAndWheelScrollLatching,
             features::kAsyncWheelEvents},
            {});
    }

    queue_.reset(new MouseWheelEventQueue(this, scroll_latching_enabled_));
  }

  ~MouseWheelEventQueueTest() override {}

  // MouseWheelEventQueueClient
  void SendMouseWheelEventImmediately(
      const MouseWheelEventWithLatencyInfo& event) override {
    WebMouseWheelEvent* cloned_event = new WebMouseWheelEvent();
    std::unique_ptr<WebInputEvent> cloned_event_holder(cloned_event);
    *cloned_event = event.event;
    sent_events_.push_back(std::move(cloned_event_holder));
  }

  void ForwardGestureEventWithLatencyInfo(
      const blink::WebGestureEvent& event,
      const ui::LatencyInfo& latency_info) override {
    WebGestureEvent* cloned_event = new WebGestureEvent();
    std::unique_ptr<WebInputEvent> cloned_event_holder(cloned_event);
    *cloned_event = event;
    if (event.GetType() == WebInputEvent::kGestureScrollBegin) {
      is_wheel_scroll_in_progress_ = true;
    } else if (event.GetType() == WebInputEvent::kGestureScrollEnd) {
      is_wheel_scroll_in_progress_ = false;
    }
    sent_events_.push_back(std::move(cloned_event_holder));
  }

  void OnMouseWheelEventAck(const MouseWheelEventWithLatencyInfo& event,
                            InputEventAckSource ack_source,
                            InputEventAckState ack_result) override {
    ++acked_event_count_;
    last_acked_event_ = event.event;
    last_acked_event_state_ = ack_result;
  }

  bool IsWheelScrollInProgress() override {
    return is_wheel_scroll_in_progress_;
  }

  bool scroll_latching_enabled() { return scroll_latching_enabled_; }

 protected:
  size_t queued_event_count() const { return queue_->queued_size(); }

  bool event_in_flight() const { return queue_->event_in_flight(); }

  std::vector<std::unique_ptr<WebInputEvent>>& all_sent_events() {
    return sent_events_;
  }

  const std::unique_ptr<WebInputEvent>& sent_input_event(size_t index) {
    return sent_events_[index];
  }
  const WebGestureEvent* sent_gesture_event(size_t index) {
    return static_cast<WebGestureEvent*>(sent_events_[index].get());
  }

  const WebMouseWheelEvent& acked_event() const { return last_acked_event_; }

  size_t GetAndResetSentEventCount() {
    size_t count = sent_events_.size();
    sent_events_.clear();
    return count;
  }

  size_t GetAndResetAckedEventCount() {
    size_t count = acked_event_count_;
    acked_event_count_ = 0;
    return count;
  }

  void SendMouseWheelEventAck(InputEventAckState ack_result) {
    queue_->ProcessMouseWheelAck(InputEventAckSource::COMPOSITOR_THREAD,
                                 ack_result, ui::LatencyInfo());
  }

  void SendMouseWheel(float x,
                      float y,
                      float global_x,
                      float global_y,
                      float dX,
                      float dY,
                      int modifiers,
                      bool high_precision,
                      WebInputEvent::RailsMode rails_mode) {
    WebMouseWheelEvent event = SyntheticWebMouseWheelEventBuilder::Build(
        x, y, global_x, global_y, dX, dY, modifiers, high_precision);
    event.rails_mode = rails_mode;
    queue_->QueueEvent(MouseWheelEventWithLatencyInfo(event));
  }

  void SendMouseWheel(float x,
                      float y,
                      float global_x,
                      float global_y,
                      float dX,
                      float dY,
                      int modifiers,
                      bool high_precision) {
    SendMouseWheel(x, y, global_x, global_y, dX, dY, modifiers, high_precision,
                   WebInputEvent::kRailsModeFree);
  }
  void SendMouseWheelWithPhase(float x,
                               float y,
                               float global_x,
                               float global_y,
                               float dX,
                               float dY,
                               int modifiers,
                               bool high_precision,
                               blink::WebMouseWheelEvent::Phase phase,
                               blink::WebMouseWheelEvent::Phase momentum_phase,
                               WebInputEvent::RailsMode rails_mode) {
    WebMouseWheelEvent event = SyntheticWebMouseWheelEventBuilder::Build(
        x, y, global_x, global_y, dX, dY, modifiers, high_precision);
    event.phase = phase;
    event.momentum_phase = momentum_phase;
    event.rails_mode = rails_mode;
    queue_->QueueEvent(MouseWheelEventWithLatencyInfo(event));
  }
  void SendMouseWheelWithPhase(
      float x,
      float y,
      float global_x,
      float global_y,
      float dX,
      float dY,
      int modifiers,
      bool high_precision,
      blink::WebMouseWheelEvent::Phase phase,
      blink::WebMouseWheelEvent::Phase momentum_phase) {
    SendMouseWheelWithPhase(x, y, global_x, global_y, dX, dY, modifiers,
                            high_precision, phase, momentum_phase,
                            WebInputEvent::kRailsModeFree);
  }

  void SendMouseWheelPossiblyIncludingPhase(
      bool ignore_phase,
      float x,
      float y,
      float global_x,
      float global_y,
      float dX,
      float dY,
      int modifiers,
      bool high_precision,
      blink::WebMouseWheelEvent::Phase phase,
      blink::WebMouseWheelEvent::Phase momentum_phase,
      WebInputEvent::RailsMode rails_mode) {
    if (ignore_phase) {
      SendMouseWheel(x, y, global_x, global_y, dX, dY, modifiers,
                     high_precision, rails_mode);
    } else {
      SendMouseWheelWithPhase(x, y, global_x, global_y, dX, dY, modifiers,
                              high_precision, phase, momentum_phase,
                              rails_mode);
    }
  }

  void SendMouseWheelPossiblyIncludingPhase(
      bool ignore_phase,
      float x,
      float y,
      float global_x,
      float global_y,
      float dX,
      float dY,
      int modifiers,
      bool high_precision,
      blink::WebMouseWheelEvent::Phase phase,
      blink::WebMouseWheelEvent::Phase momentum_phase) {
    SendMouseWheelPossiblyIncludingPhase(
        ignore_phase, x, y, global_x, global_y, dX, dY, modifiers,
        high_precision, phase, momentum_phase, WebInputEvent::kRailsModeFree);
  }

  void SendGestureEvent(WebInputEvent::Type type) {
    WebGestureEvent event(type, WebInputEvent::kNoModifiers,
                          ui::EventTimeForNow(),
                          blink::kWebGestureDeviceTouchscreen);
    queue_->OnGestureScrollEvent(
        GestureEventWithLatencyInfo(event, ui::LatencyInfo()));
  }

  static void RunTasksAndWait(base::TimeDelta delay) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated(),
        delay);
    base::RunLoop().Run();
  }

  void GestureSendingTest(bool high_precision) {
    const WebGestureEvent::ScrollUnits scroll_units =
        high_precision ? WebGestureEvent::kPrecisePixels
                       : WebGestureEvent::kPixels;
    SendMouseWheelPossiblyIncludingPhase(
        !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
        kWheelScrollGlobalX, kWheelScrollGlobalY, 1, 1, 0, high_precision,
        WebMouseWheelEvent::kPhaseBegan, WebMouseWheelEvent::kPhaseNone);
    EXPECT_EQ(0U, queued_event_count());
    EXPECT_TRUE(event_in_flight());
    EXPECT_EQ(1U, GetAndResetSentEventCount());

    // The second mouse wheel should not be sent since one is already in
    // queue.
    SendMouseWheelPossiblyIncludingPhase(
        !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
        kWheelScrollGlobalX, kWheelScrollGlobalY, 5, 5, 0, high_precision,
        WebMouseWheelEvent::kPhaseChanged, WebMouseWheelEvent::kPhaseNone);
    EXPECT_EQ(1U, queued_event_count());
    EXPECT_TRUE(event_in_flight());
    EXPECT_EQ(0U, GetAndResetSentEventCount());

    // Receive an ACK for the mouse wheel event and release the next
    // mouse wheel event.
    SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
    EXPECT_EQ(0U, queued_event_count());
    EXPECT_TRUE(event_in_flight());
    EXPECT_EQ(WebInputEvent::kMouseWheel, acked_event().GetType());
    EXPECT_EQ(1U, GetAndResetAckedEventCount());
    if (scroll_latching_enabled_) {
      EXPECT_EQ(3U, all_sent_events().size());
      EXPECT_GESTURE_SCROLL_BEGIN_WITH_PHASE(sent_gesture_event(0));
      EXPECT_GESTURE_SCROLL_UPDATE_WITH_PHASE(sent_gesture_event(1));
      EXPECT_MOUSE_WHEEL(sent_input_event(2));
      EXPECT_EQ(3U, GetAndResetSentEventCount());
    } else {
      EXPECT_EQ(4U, all_sent_events().size());
      EXPECT_GESTURE_SCROLL_BEGIN(sent_gesture_event(0));
      EXPECT_GESTURE_SCROLL_UPDATE(sent_gesture_event(1));
      EXPECT_GESTURE_SCROLL_END(sent_gesture_event(2));
      EXPECT_MOUSE_WHEEL(sent_input_event(3));
      EXPECT_EQ(4U, GetAndResetSentEventCount());
    }
  }

  void PhaseGestureSendingTest(bool high_precision) {
    const WebGestureEvent::ScrollUnits scroll_units =
        high_precision ? WebGestureEvent::kPrecisePixels
                       : WebGestureEvent::kPixels;

    SendMouseWheelWithPhase(kWheelScrollX, kWheelScrollY, kWheelScrollGlobalX,
                            kWheelScrollGlobalY, 1, 1, 0, high_precision,
                            WebMouseWheelEvent::kPhaseBegan,
                            WebMouseWheelEvent::kPhaseNone);
    EXPECT_EQ(1U, GetAndResetSentEventCount());
    SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
    if (scroll_latching_enabled_) {
      EXPECT_EQ(2U, all_sent_events().size());
      EXPECT_GESTURE_SCROLL_BEGIN_WITH_PHASE(sent_gesture_event(0));
      EXPECT_GESTURE_SCROLL_UPDATE_WITH_PHASE(sent_gesture_event(1));
      EXPECT_EQ(2U, GetAndResetSentEventCount());
    } else {
      EXPECT_EQ(3U, all_sent_events().size());
      EXPECT_GESTURE_SCROLL_BEGIN_WITH_PHASE(sent_gesture_event(0));
      EXPECT_GESTURE_SCROLL_UPDATE_WITH_PHASE(sent_gesture_event(1));
      EXPECT_SYNTHETIC_GESTURE_SCROLL_END(sent_gesture_event(2));
      EXPECT_EQ(3U, GetAndResetSentEventCount());
    }

    SendMouseWheelWithPhase(kWheelScrollX, kWheelScrollY, kWheelScrollGlobalX,
                            kWheelScrollGlobalY, 5, 5, 0, high_precision,
                            WebMouseWheelEvent::kPhaseChanged,
                            WebMouseWheelEvent::kPhaseNone);
    EXPECT_EQ(1U, GetAndResetSentEventCount());
    SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
    if (scroll_latching_enabled_) {
      EXPECT_EQ(1U, all_sent_events().size());
      EXPECT_GESTURE_SCROLL_UPDATE_WITH_PHASE(sent_gesture_event(0));
      EXPECT_EQ(1U, GetAndResetSentEventCount());
    } else {
      EXPECT_EQ(3U, all_sent_events().size());
      EXPECT_SYNTHETIC_GESTURE_SCROLL_BEGIN(sent_gesture_event(0));
      EXPECT_GESTURE_SCROLL_UPDATE_WITH_PHASE(sent_gesture_event(1));
      EXPECT_SYNTHETIC_GESTURE_SCROLL_END(sent_gesture_event(2));
      EXPECT_EQ(3U, GetAndResetSentEventCount());
    }

    // When wheel scroll latching is enabled no wheel event with phase =
    // |kPhaseEnded| will be sent before a wheel event with momentum_phase =
    // |kPhaseBegan|.
    if (!scroll_latching_enabled_) {
      SendMouseWheelWithPhase(kWheelScrollX, kWheelScrollY, kWheelScrollGlobalX,
                              kWheelScrollGlobalY, 0, 0, 0, high_precision,
                              WebMouseWheelEvent::kPhaseEnded,
                              WebMouseWheelEvent::kPhaseNone);
      EXPECT_EQ(1U, GetAndResetSentEventCount());
      SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
      EXPECT_EQ(2U, all_sent_events().size());
      EXPECT_SYNTHETIC_GESTURE_SCROLL_BEGIN(sent_gesture_event(0));
      EXPECT_GESTURE_SCROLL_END_WITH_PHASE(sent_gesture_event(1));
      EXPECT_EQ(2U, GetAndResetSentEventCount());

      // Send a double phase end; OSX does it consistently.
      SendMouseWheelWithPhase(kWheelScrollX, kWheelScrollY, kWheelScrollGlobalX,
                              kWheelScrollGlobalY, 0, 0, 0, high_precision,
                              WebMouseWheelEvent::kPhaseEnded,
                              WebMouseWheelEvent::kPhaseNone);
      EXPECT_EQ(1U, GetAndResetSentEventCount());
      SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
      EXPECT_EQ(0U, all_sent_events().size());
      EXPECT_EQ(0U, GetAndResetSentEventCount());
    }

    SendMouseWheelWithPhase(kWheelScrollX, kWheelScrollY, kWheelScrollGlobalX,
                            kWheelScrollGlobalY, 5, 5, 0, high_precision,
                            WebMouseWheelEvent::kPhaseNone,
                            WebMouseWheelEvent::kPhaseBegan);
    EXPECT_EQ(1U, GetAndResetSentEventCount());
    SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
    if (scroll_latching_enabled_) {
      // A fling has started, no ScrollEnd/ScrollBegin is sent.
      EXPECT_EQ(1U, all_sent_events().size());
      EXPECT_INERTIAL_GESTURE_SCROLL_UPDATE(sent_gesture_event(0));
      EXPECT_EQ(1U, GetAndResetSentEventCount());
    } else {
      EXPECT_EQ(3U, all_sent_events().size());
      EXPECT_INERTIAL_GESTURE_SCROLL_BEGIN(sent_gesture_event(0));
      EXPECT_INERTIAL_GESTURE_SCROLL_UPDATE(sent_gesture_event(1));
      EXPECT_SYNTHETIC_INERTIAL_GESTURE_SCROLL_END(sent_gesture_event(2));
      EXPECT_EQ(3U, GetAndResetSentEventCount());
    }

    SendMouseWheelWithPhase(kWheelScrollX, kWheelScrollY, kWheelScrollGlobalX,
                            kWheelScrollGlobalY, 5, 5, 0, high_precision,
                            WebMouseWheelEvent::kPhaseNone,
                            WebMouseWheelEvent::kPhaseChanged);
    EXPECT_EQ(1U, GetAndResetSentEventCount());
    SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
    if (scroll_latching_enabled_) {
      EXPECT_EQ(1U, all_sent_events().size());
      EXPECT_INERTIAL_GESTURE_SCROLL_UPDATE(sent_gesture_event(0));
      EXPECT_EQ(1U, GetAndResetSentEventCount());
    } else {
      EXPECT_EQ(3U, all_sent_events().size());
      EXPECT_SYNTHETIC_INERTIAL_GESTURE_SCROLL_BEGIN(sent_gesture_event(0));
      EXPECT_INERTIAL_GESTURE_SCROLL_UPDATE(sent_gesture_event(1));
      EXPECT_SYNTHETIC_INERTIAL_GESTURE_SCROLL_END(sent_gesture_event(2));
      EXPECT_EQ(3U, GetAndResetSentEventCount());
    }

    SendMouseWheelWithPhase(kWheelScrollX, kWheelScrollY, kWheelScrollGlobalX,
                            kWheelScrollGlobalY, 0, 0, 0, high_precision,
                            WebMouseWheelEvent::kPhaseNone,
                            WebMouseWheelEvent::kPhaseEnded);
    EXPECT_EQ(1U, GetAndResetSentEventCount());
    SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
    if (scroll_latching_enabled_) {
      // MomentumPhase is ended, the scroll is done, and GSE is sent
      // immediately.
      EXPECT_EQ(1U, all_sent_events().size());
      EXPECT_INERTIAL_GESTURE_SCROLL_END(sent_gesture_event(0));
      EXPECT_EQ(1U, GetAndResetSentEventCount());
    } else {
      EXPECT_EQ(2U, all_sent_events().size());
      EXPECT_SYNTHETIC_INERTIAL_GESTURE_SCROLL_BEGIN(sent_gesture_event(0));
      EXPECT_INERTIAL_GESTURE_SCROLL_END(sent_gesture_event(1));
      EXPECT_EQ(2U, GetAndResetSentEventCount());
    }
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<MouseWheelEventQueue> queue_;
  std::vector<std::unique_ptr<WebInputEvent>> sent_events_;
  size_t acked_event_count_;
  InputEventAckState last_acked_event_state_;
  WebMouseWheelEvent last_acked_event_;
  bool scroll_latching_enabled_;

 private:
  bool is_wheel_scroll_in_progress_ = false;
  base::test::ScopedFeatureList feature_list_;
};

// Tests that mouse wheel events are queued properly.
TEST_P(MouseWheelEventQueueTest, Basic) {
  SendMouseWheelPossiblyIncludingPhase(
      !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
      kWheelScrollGlobalX, kWheelScrollGlobalY, 1, 1, 0, false,
      WebMouseWheelEvent::kPhaseBegan, WebMouseWheelEvent::kPhaseNone);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_TRUE(event_in_flight());
  EXPECT_EQ(1U, GetAndResetSentEventCount());

  // The second mouse wheel should not be sent since one is already in queue.
  SendMouseWheelPossiblyIncludingPhase(
      !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
      kWheelScrollGlobalX, kWheelScrollGlobalY, 5, 5, 0, false,
      WebMouseWheelEvent::kPhaseChanged, WebMouseWheelEvent::kPhaseNone);
  EXPECT_EQ(1U, queued_event_count());
  EXPECT_TRUE(event_in_flight());
  EXPECT_EQ(0U, GetAndResetSentEventCount());

  // Receive an ACK for the first mouse wheel event.
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_CONSUMED);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_TRUE(event_in_flight());
  EXPECT_EQ(1U, GetAndResetSentEventCount());
  EXPECT_EQ(1U, GetAndResetAckedEventCount());
  EXPECT_EQ(WebInputEvent::kMouseWheel, acked_event().GetType());

  // Receive an ACK for the second mouse wheel event.
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_CONSUMED);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_FALSE(event_in_flight());
  EXPECT_EQ(0U, GetAndResetSentEventCount());
  EXPECT_EQ(1U, GetAndResetAckedEventCount());
  EXPECT_EQ(WebInputEvent::kMouseWheel, acked_event().GetType());
}

TEST_P(MouseWheelEventQueueTest, GestureSending) {
  GestureSendingTest(false);
}

TEST_P(MouseWheelEventQueueTest, GestureSendingPrecisePixels) {
  GestureSendingTest(true);
}

TEST_P(MouseWheelEventQueueTest, GestureSendingWithPhaseInformation) {
  PhaseGestureSendingTest(false);
}

TEST_P(MouseWheelEventQueueTest,
       GestureSendingWithPhaseInformationPrecisePixels) {
  PhaseGestureSendingTest(true);
}

TEST_P(MouseWheelEventQueueTest, GestureSendingInterrupted) {
  const WebGestureEvent::ScrollUnits scroll_units = WebGestureEvent::kPixels;
  SendMouseWheelPossiblyIncludingPhase(
      !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
      kWheelScrollGlobalX, kWheelScrollGlobalY, 1, 1, 0, false,
      WebMouseWheelEvent::kPhaseBegan, WebMouseWheelEvent::kPhaseNone);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_TRUE(event_in_flight());
  EXPECT_EQ(1U, GetAndResetSentEventCount());

  // Receive an ACK for the mouse wheel event.
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_FALSE(event_in_flight());
  EXPECT_EQ(WebInputEvent::kMouseWheel, acked_event().GetType());
  EXPECT_EQ(1U, GetAndResetAckedEventCount());
  if (scroll_latching_enabled_) {
    EXPECT_EQ(2U, all_sent_events().size());
    EXPECT_GESTURE_SCROLL_BEGIN_WITH_PHASE(sent_gesture_event(0));
    EXPECT_GESTURE_SCROLL_UPDATE_WITH_PHASE(sent_gesture_event(1));
    EXPECT_EQ(2U, GetAndResetSentEventCount());
  } else {
    EXPECT_EQ(3U, all_sent_events().size());
    EXPECT_GESTURE_SCROLL_BEGIN(sent_gesture_event(0));
    EXPECT_GESTURE_SCROLL_UPDATE(sent_gesture_event(1));
    EXPECT_GESTURE_SCROLL_END(sent_gesture_event(2));
    EXPECT_EQ(3U, GetAndResetSentEventCount());
  }

  // When wheel scroll latching is enabled and a touch based GSB arrives in the
  // middle of wheel scrolling sequence, a synthetic wheel event with zero
  // deltas and phase = |kPhaseEnded| will be sent.
  if (scroll_latching_enabled_) {
    SendMouseWheelWithPhase(kWheelScrollX, kWheelScrollY, kWheelScrollGlobalX,
                            kWheelScrollGlobalY, 0, 0, 0, false,
                            WebMouseWheelEvent::kPhaseEnded,
                            WebMouseWheelEvent::kPhaseNone);
    SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
    EXPECT_EQ(1U, GetAndResetAckedEventCount());
  }
  // Ensure that a gesture scroll begin terminates the current scroll event.
  SendGestureEvent(WebInputEvent::kGestureScrollBegin);

  if (scroll_latching_enabled_) {
    EXPECT_EQ(2U, all_sent_events().size());
    EXPECT_GESTURE_SCROLL_END_WITH_PHASE(sent_gesture_event(1));
    EXPECT_EQ(2U, GetAndResetSentEventCount());
  } else {
    // ScrollEnd has already been sent.
    EXPECT_EQ(0U, all_sent_events().size());
  }

  SendMouseWheelPossiblyIncludingPhase(
      !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
      kWheelScrollGlobalX, kWheelScrollGlobalY, 1, 1, 0, false,
      WebMouseWheelEvent::kPhaseBegan, WebMouseWheelEvent::kPhaseNone);

  EXPECT_EQ(0U, queued_event_count());
  EXPECT_TRUE(event_in_flight());
  EXPECT_EQ(1U, GetAndResetSentEventCount());

  // New mouse wheel events won't cause gestures because a scroll
  // is already in progress by another device.
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_FALSE(event_in_flight());
  EXPECT_EQ(WebInputEvent::kMouseWheel, acked_event().GetType());
  EXPECT_EQ(1U, GetAndResetAckedEventCount());
  EXPECT_EQ(0U, all_sent_events().size());

  SendGestureEvent(WebInputEvent::kGestureScrollEnd);
  EXPECT_EQ(0U, all_sent_events().size());

  SendMouseWheelPossiblyIncludingPhase(
      !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
      kWheelScrollGlobalX, kWheelScrollGlobalY, 1, 1, 0, false,
      WebMouseWheelEvent::kPhaseBegan, WebMouseWheelEvent::kPhaseNone);

  EXPECT_EQ(0U, queued_event_count());
  EXPECT_TRUE(event_in_flight());
  EXPECT_EQ(1U, GetAndResetSentEventCount());

  // Receive an ACK for the mouse wheel event.
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_FALSE(event_in_flight());
  EXPECT_EQ(WebInputEvent::kMouseWheel, acked_event().GetType());
  EXPECT_EQ(1U, GetAndResetAckedEventCount());
  if (scroll_latching_enabled_) {
    EXPECT_EQ(2U, all_sent_events().size());
    EXPECT_GESTURE_SCROLL_BEGIN_WITH_PHASE(sent_gesture_event(0));
    EXPECT_GESTURE_SCROLL_UPDATE_WITH_PHASE(sent_gesture_event(1));
    EXPECT_EQ(2U, GetAndResetSentEventCount());
  } else {
    EXPECT_EQ(3U, all_sent_events().size());
    EXPECT_GESTURE_SCROLL_BEGIN(sent_gesture_event(0));
    EXPECT_GESTURE_SCROLL_UPDATE(sent_gesture_event(1));
    EXPECT_GESTURE_SCROLL_END(sent_gesture_event(2));
    EXPECT_EQ(3U, GetAndResetSentEventCount());
  }
}

TEST_P(MouseWheelEventQueueTest, GestureRailScrolling) {
  const WebGestureEvent::ScrollUnits scroll_units = WebGestureEvent::kPixels;

  SendMouseWheelPossiblyIncludingPhase(
      !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
      kWheelScrollGlobalX, kWheelScrollGlobalY, 1, 1, 0, false,
      WebMouseWheelEvent::kPhaseBegan, WebMouseWheelEvent::kPhaseNone,
      WebInputEvent::kRailsModeHorizontal);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_TRUE(event_in_flight());
  EXPECT_EQ(1U, GetAndResetSentEventCount());

  // Receive an ACK for the mouse wheel event.
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_FALSE(event_in_flight());
  EXPECT_EQ(WebInputEvent::kMouseWheel, acked_event().GetType());
  EXPECT_EQ(1U, GetAndResetAckedEventCount());
  if (scroll_latching_enabled_) {
    EXPECT_EQ(2U, all_sent_events().size());
    EXPECT_GESTURE_SCROLL_BEGIN_WITH_PHASE(sent_gesture_event(0));
    EXPECT_GESTURE_SCROLL_UPDATE_WITH_PHASE(sent_gesture_event(1));
    EXPECT_EQ(1U, sent_gesture_event(1)->data.scroll_update.delta_x);
    EXPECT_EQ(0U, sent_gesture_event(1)->data.scroll_update.delta_y);
    EXPECT_EQ(2U, GetAndResetSentEventCount());
  } else {
    EXPECT_EQ(3U, all_sent_events().size());
    EXPECT_GESTURE_SCROLL_BEGIN(sent_gesture_event(0));
    EXPECT_GESTURE_SCROLL_UPDATE(sent_gesture_event(1));
    EXPECT_GESTURE_SCROLL_END(sent_gesture_event(2));
    EXPECT_EQ(1U, sent_gesture_event(1)->data.scroll_update.delta_x);
    EXPECT_EQ(0U, sent_gesture_event(1)->data.scroll_update.delta_y);
    EXPECT_EQ(3U, GetAndResetSentEventCount());
  }
  SendMouseWheelPossiblyIncludingPhase(
      !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
      kWheelScrollGlobalX, kWheelScrollGlobalY, 1, 1, 0, false,
      WebMouseWheelEvent::kPhaseChanged, WebMouseWheelEvent::kPhaseNone,
      WebInputEvent::kRailsModeVertical);

  EXPECT_EQ(0U, queued_event_count());
  EXPECT_TRUE(event_in_flight());
  EXPECT_EQ(1U, GetAndResetSentEventCount());

  // Receive an ACK for the mouse wheel event.
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_FALSE(event_in_flight());
  EXPECT_EQ(WebInputEvent::kMouseWheel, acked_event().GetType());
  EXPECT_EQ(1U, GetAndResetAckedEventCount());
  size_t scroll_update_index = 0;
  if (scroll_latching_enabled_) {
    EXPECT_EQ(1U, all_sent_events().size());
    EXPECT_GESTURE_SCROLL_UPDATE_WITH_PHASE(sent_gesture_event(0));
  } else {
    EXPECT_EQ(3U, all_sent_events().size());
    EXPECT_GESTURE_SCROLL_BEGIN(sent_gesture_event(0));
    EXPECT_GESTURE_SCROLL_UPDATE(sent_gesture_event(1));
    EXPECT_GESTURE_SCROLL_END(sent_gesture_event(2));
    scroll_update_index = 1;
  }
  EXPECT_EQ(
      0U, sent_gesture_event(scroll_update_index)->data.scroll_update.delta_x);
  EXPECT_EQ(
      1U, sent_gesture_event(scroll_update_index)->data.scroll_update.delta_y);
  if (scroll_latching_enabled_)
    EXPECT_EQ(1U, GetAndResetSentEventCount());
  else
    EXPECT_EQ(3U, GetAndResetSentEventCount());
}

TEST_P(MouseWheelEventQueueTest, WheelScrollLatching) {
  if (!scroll_latching_enabled_)
    return;

  const WebGestureEvent::ScrollUnits scroll_units = WebGestureEvent::kPixels;
  SendMouseWheelWithPhase(
      kWheelScrollX, kWheelScrollY, kWheelScrollGlobalX, kWheelScrollGlobalY, 1,
      1, 0, false, WebMouseWheelEvent::kPhaseBegan,
      WebMouseWheelEvent::kPhaseNone, WebInputEvent::kRailsModeVertical);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_TRUE(event_in_flight());
  EXPECT_EQ(1U, GetAndResetSentEventCount());

  // Receive an ACK for the mouse wheel event.
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_FALSE(event_in_flight());
  EXPECT_EQ(WebInputEvent::kMouseWheel, acked_event().GetType());
  EXPECT_EQ(1U, GetAndResetAckedEventCount());
  EXPECT_EQ(2U, all_sent_events().size());
  EXPECT_GESTURE_SCROLL_BEGIN_WITH_PHASE(sent_gesture_event(0));
  EXPECT_GESTURE_SCROLL_UPDATE_WITH_PHASE(sent_gesture_event(1));
  EXPECT_EQ(0U, sent_gesture_event(1)->data.scroll_update.delta_x);
  EXPECT_EQ(1U, sent_gesture_event(1)->data.scroll_update.delta_y);
  EXPECT_EQ(2U, GetAndResetSentEventCount());

  SendMouseWheelWithPhase(
      kWheelScrollX, kWheelScrollY, kWheelScrollGlobalX, kWheelScrollGlobalY, 1,
      1, 0, false, WebMouseWheelEvent::kPhaseChanged,
      WebMouseWheelEvent::kPhaseNone, WebInputEvent::kRailsModeVertical);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_TRUE(event_in_flight());
  EXPECT_EQ(1U, GetAndResetSentEventCount());

  // Receive an ACK for the mouse wheel event.
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_FALSE(event_in_flight());
  EXPECT_EQ(WebInputEvent::kMouseWheel, acked_event().GetType());
  EXPECT_EQ(1U, GetAndResetAckedEventCount());

  // Scroll latching: no new scroll begin expected.
  EXPECT_EQ(1U, all_sent_events().size());
  EXPECT_GESTURE_SCROLL_UPDATE_WITH_PHASE(sent_gesture_event(0));
  EXPECT_EQ(0U, sent_gesture_event(0)->data.scroll_update.delta_x);
  EXPECT_EQ(1U, sent_gesture_event(0)->data.scroll_update.delta_y);
  EXPECT_EQ(1U, GetAndResetSentEventCount());
}

TEST_P(MouseWheelEventQueueTest, WheelScrollingWasLatchedHistogramCheck) {
  base::HistogramTester histogram_tester;
  const char latching_histogram_name[] = "WheelScrolling.WasLatched";

  SendMouseWheelPossiblyIncludingPhase(
      !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
      kWheelScrollGlobalX, kWheelScrollGlobalY, 1, 1, 0, false,
      WebMouseWheelEvent::kPhaseBegan, WebMouseWheelEvent::kPhaseNone);
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
  histogram_tester.ExpectBucketCount(latching_histogram_name, 0, 1);

  SendMouseWheelPossiblyIncludingPhase(
      !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
      kWheelScrollGlobalX, kWheelScrollGlobalY, 1, 1, 0, false,
      WebMouseWheelEvent::kPhaseChanged, WebMouseWheelEvent::kPhaseNone);
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_NOT_CONSUMED);
  if (scroll_latching_enabled_) {
    histogram_tester.ExpectBucketCount(latching_histogram_name, 0, 1);
    histogram_tester.ExpectBucketCount(latching_histogram_name, 1, 1);
  } else {
    histogram_tester.ExpectBucketCount(latching_histogram_name, 0, 2);
  }
}

INSTANTIATE_TEST_CASE_P(MouseWheelEventQueueTests,
                        MouseWheelEventQueueTest,
                        testing::Values(kWheelScrollingModeNone,
                                        kWheelScrollLatching,
                                        kAsyncWheelEvents));

}  // namespace content
