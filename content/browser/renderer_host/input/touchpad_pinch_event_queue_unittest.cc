// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/touchpad_pinch_event_queue.h"

#include <string>

#include "content/common/input/event_with_latency_info.h"
#include "content/public/common/input_event_ack_source.h"
#include "content/public/common/input_event_ack_state.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/latency/latency_info.h"

namespace content {

class MockTouchpadPinchEventQueueClient : public TouchpadPinchEventQueueClient {
 public:
  MockTouchpadPinchEventQueueClient() = default;
  ~MockTouchpadPinchEventQueueClient() override = default;

  // TouchpadPinchEventQueueClient
  MOCK_METHOD1(SendMouseWheelEventForPinchImmediately,
               void(const MouseWheelEventWithLatencyInfo& event));
  MOCK_METHOD3(OnGestureEventForPinchAck,
               void(const GestureEventWithLatencyInfo& event,
                    InputEventAckSource ack_source,
                    InputEventAckState ack_result));
};

class TouchpadPinchEventQueueTest : public ::testing::Test {
 protected:
  TouchpadPinchEventQueueTest() : queue_(&mock_client_) {}
  ~TouchpadPinchEventQueueTest() = default;

  void QueueEvent(const blink::WebGestureEvent& event) {
    queue_.QueueEvent(GestureEventWithLatencyInfo(event));
  }

  void QueuePinchBegin() {
    blink::WebGestureEvent event(
        blink::WebInputEvent::kGesturePinchBegin,
        blink::WebInputEvent::kNoModifiers,
        blink::WebInputEvent::GetStaticTimeStampForTests(),
        blink::kWebGestureDeviceTouchpad);
    event.SetPositionInWidget(gfx::PointF(1, 1));
    event.SetPositionInScreen(gfx::PointF(1, 1));
    event.SetNeedsWheelEvent(true);
    QueueEvent(event);
  }

  void QueuePinchEnd() {
    blink::WebGestureEvent event(
        blink::WebInputEvent::kGesturePinchEnd,
        blink::WebInputEvent::kNoModifiers,
        blink::WebInputEvent::GetStaticTimeStampForTests(),
        blink::kWebGestureDeviceTouchpad);
    event.SetPositionInWidget(gfx::PointF(1, 1));
    event.SetPositionInScreen(gfx::PointF(1, 1));
    event.SetNeedsWheelEvent(true);
    QueueEvent(event);
  }

  void QueuePinchUpdate(float scale, bool zoom_disabled) {
    blink::WebGestureEvent event(
        blink::WebInputEvent::kGesturePinchUpdate,
        blink::WebInputEvent::kNoModifiers,
        blink::WebInputEvent::GetStaticTimeStampForTests(),
        blink::kWebGestureDeviceTouchpad);
    event.SetPositionInWidget(gfx::PointF(1, 1));
    event.SetPositionInScreen(gfx::PointF(1, 1));
    event.SetNeedsWheelEvent(true);
    event.data.pinch_update.zoom_disabled = zoom_disabled;
    event.data.pinch_update.scale = scale;
    QueueEvent(event);
  }

  void SendWheelEventAck(InputEventAckSource ack_source,
                         InputEventAckState ack_result) {
    queue_.ProcessMouseWheelAck(ack_source, ack_result, ui::LatencyInfo());
  }

  testing::StrictMock<MockTouchpadPinchEventQueueClient> mock_client_;
  TouchpadPinchEventQueue queue_;
};

MATCHER_P(EventHasType,
          type,
          std::string(negation ? "does not have" : "has") + " type " +
              ::testing::PrintToString(type)) {
  return arg.event.GetType() == type;
}

MATCHER(EventHasCtrlModifier,
        std::string(negation ? "does not have" : "has") + " control modifier") {
  return (arg.event.GetModifiers() & blink::WebInputEvent::kControlKey) != 0;
}

MATCHER(EventIsBlocking,
        std::string(negation ? "is not" : "is") + " blocking") {
  return arg.event.dispatch_type == blink::WebInputEvent::kBlocking;
}

// Ensure that when the queue receives a touchpad pinch sequence, it sends a
// synthetic mouse wheel event and acks the pinch events back to the client.
TEST_F(TouchpadPinchEventQueueTest, Basic) {
  ::testing::InSequence sequence;
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchBegin),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));
  EXPECT_CALL(mock_client_,
              SendMouseWheelEventForPinchImmediately(
                  ::testing::AllOf(EventHasCtrlModifier(), EventIsBlocking())));
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchUpdate),
                  InputEventAckSource::COMPOSITOR_THREAD,
                  INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS));
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchEnd),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));

  QueuePinchBegin();
  QueuePinchUpdate(1.23, false);
  SendWheelEventAck(InputEventAckSource::COMPOSITOR_THREAD,
                    INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS);
  QueuePinchEnd();
}

// Ensure that if the renderer consumes the synthetic wheel event, the ack of
// the GesturePinchUpdate reflects this.
TEST_F(TouchpadPinchEventQueueTest, Consumed) {
  ::testing::InSequence sequence;
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchBegin),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));
  EXPECT_CALL(mock_client_,
              SendMouseWheelEventForPinchImmediately(
                  ::testing::AllOf(EventHasCtrlModifier(), EventIsBlocking())));
  EXPECT_CALL(
      mock_client_,
      OnGestureEventForPinchAck(
          EventHasType(blink::WebInputEvent::kGesturePinchUpdate),
          InputEventAckSource::MAIN_THREAD, INPUT_EVENT_ACK_STATE_CONSUMED));
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchEnd),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));

  QueuePinchBegin();
  QueuePinchUpdate(1.23, false);
  SendWheelEventAck(InputEventAckSource::MAIN_THREAD,
                    INPUT_EVENT_ACK_STATE_CONSUMED);
  QueuePinchEnd();
}

// Ensure that the queue sends wheel events for updates with |zoom_disabled| as
// well.
TEST_F(TouchpadPinchEventQueueTest, ZoomDisabled) {
  ::testing::InSequence sequence;
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchBegin),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));
  EXPECT_CALL(mock_client_,
              SendMouseWheelEventForPinchImmediately(
                  ::testing::AllOf(EventHasCtrlModifier(), EventIsBlocking())));
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchUpdate),
                  InputEventAckSource::COMPOSITOR_THREAD,
                  INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS));
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchEnd),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));

  QueuePinchBegin();
  QueuePinchUpdate(1.23, true);
  SendWheelEventAck(InputEventAckSource::COMPOSITOR_THREAD,
                    INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS);
  QueuePinchEnd();
}

TEST_F(TouchpadPinchEventQueueTest, MultipleSequences) {
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchBegin),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED))
      .Times(2);
  EXPECT_CALL(mock_client_, SendMouseWheelEventForPinchImmediately(testing::_))
      .Times(2);
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchUpdate),
                  InputEventAckSource::COMPOSITOR_THREAD,
                  INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS))
      .Times(2);
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchEnd),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED))
      .Times(2);

  QueuePinchBegin();
  QueuePinchUpdate(1.23, false);
  SendWheelEventAck(InputEventAckSource::COMPOSITOR_THREAD,
                    INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS);
  QueuePinchEnd();

  QueuePinchBegin();
  QueuePinchUpdate(1.23, false);
  SendWheelEventAck(InputEventAckSource::COMPOSITOR_THREAD,
                    INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS);
  QueuePinchEnd();
}

// Ensure we can queue additional pinch event sequences while the queue is
// waiting for a wheel event ack.
TEST_F(TouchpadPinchEventQueueTest, MultipleQueuedSequences) {
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchBegin),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));
  EXPECT_CALL(mock_client_, SendMouseWheelEventForPinchImmediately(testing::_));
  QueuePinchBegin();
  QueuePinchUpdate(1.23, false);

  testing::Mock::VerifyAndClearExpectations(&mock_client_);

  QueuePinchEnd();

  QueuePinchBegin();
  QueuePinchUpdate(1.23, false);
  QueuePinchEnd();

  // No calls since we're still waiting on the ack for the first wheel event.
  testing::Mock::VerifyAndClearExpectations(&mock_client_);

  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchUpdate),
                  InputEventAckSource::COMPOSITOR_THREAD,
                  INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS));
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchEnd),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchBegin),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));
  EXPECT_CALL(mock_client_, SendMouseWheelEventForPinchImmediately(testing::_));
  SendWheelEventAck(InputEventAckSource::COMPOSITOR_THREAD,
                    INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS);

  // After acking the first wheel event, the queue continues.
  testing::Mock::VerifyAndClearExpectations(&mock_client_);

  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchUpdate),
                  InputEventAckSource::COMPOSITOR_THREAD,
                  INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS));
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchEnd),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));
  SendWheelEventAck(InputEventAckSource::COMPOSITOR_THREAD,
                    INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS);
}

// Ensure the queue handles pinch event sequences with multiple updates.
TEST_F(TouchpadPinchEventQueueTest, MultipleUpdatesInSequence) {
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchBegin),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));
  EXPECT_CALL(mock_client_,
              SendMouseWheelEventForPinchImmediately(EventHasCtrlModifier()))
      .Times(3);
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchUpdate),
                  InputEventAckSource::COMPOSITOR_THREAD,
                  INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS))
      .Times(3);
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchEnd),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));

  QueuePinchBegin();
  QueuePinchUpdate(1.23, false);
  SendWheelEventAck(InputEventAckSource::COMPOSITOR_THREAD,
                    INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS);
  QueuePinchUpdate(1.23, false);
  SendWheelEventAck(InputEventAckSource::COMPOSITOR_THREAD,
                    INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS);
  QueuePinchUpdate(1.23, false);
  SendWheelEventAck(InputEventAckSource::COMPOSITOR_THREAD,
                    INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS);
  QueuePinchEnd();
}

// Ensure the queue coalesces pinch update events.
TEST_F(TouchpadPinchEventQueueTest, MultipleUpdatesCoalesced) {
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchBegin),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));
  EXPECT_CALL(mock_client_,
              SendMouseWheelEventForPinchImmediately(EventHasCtrlModifier()))
      .Times(2);
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchUpdate),
                  InputEventAckSource::COMPOSITOR_THREAD,
                  INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS))
      .Times(2);
  EXPECT_CALL(mock_client_,
              OnGestureEventForPinchAck(
                  EventHasType(blink::WebInputEvent::kGesturePinchEnd),
                  InputEventAckSource::BROWSER, INPUT_EVENT_ACK_STATE_IGNORED));

  QueuePinchBegin();
  // The queue will send the first wheel event for this first update.
  QueuePinchUpdate(1.23, false);
  // Before the first wheel event is acked, queue another update.
  QueuePinchUpdate(1.23, false);
  // Queue a third update. This will be coalesced with the second update which
  // is currently in the queue.
  QueuePinchUpdate(1.23, false);
  QueuePinchEnd();

  // Ack for the wheel event corresponding to the first update.
  SendWheelEventAck(InputEventAckSource::COMPOSITOR_THREAD,
                    INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS);
  // Ack for the wheel event corresponding to the second and third updates.
  SendWheelEventAck(InputEventAckSource::COMPOSITOR_THREAD,
                    INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS);
  EXPECT_FALSE(queue_.has_pending());
}

}  // namespace content
