// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/input_event_prediction.h"

#include "content/common/input/synthetic_web_input_event_builders.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/blink/prediction/empty_predictor.h"

using blink::WebInputEvent;
using blink::WebMouseEvent;
using blink::WebPointerProperties;
using blink::WebTouchEvent;

namespace content {

class InputEventPredictionTest : public testing::Test {
 public:
  InputEventPredictionTest() {
    event_predictor_ = std::make_unique<InputEventPrediction>();
  }

  int GetPredictorMapSize() const {
    return event_predictor_->pointer_id_predictor_map_.size();
  }

  bool GetPrediction(const WebPointerProperties& event,
                     ui::InputPredictor::InputData* result) const {
    if (!event_predictor_)
      return false;

    if (event.pointer_type == WebPointerProperties::PointerType::kMouse) {
      return event_predictor_->mouse_predictor_->GeneratePrediction(
          WebInputEvent::GetStaticTimeStampForTests(), result);
    } else {
      auto predictor =
          event_predictor_->pointer_id_predictor_map_.find(event.id);

      if (predictor != event_predictor_->pointer_id_predictor_map_.end())
        return predictor->second->GeneratePrediction(
            WebInputEvent::GetStaticTimeStampForTests(), result);
      else
        return false;
    }
  }

  void HandleEvents(const WebInputEvent& event) {
    blink::WebCoalescedInputEvent coalesced_event(event);
    event_predictor_->HandleEvents(coalesced_event,
                                   WebInputEvent::GetStaticTimeStampForTests(),
                                   coalesced_event.EventPointer());
  }

 protected:
  std::unique_ptr<InputEventPrediction> event_predictor_;

  DISALLOW_COPY_AND_ASSIGN(InputEventPredictionTest);
};

TEST_F(InputEventPredictionTest, MouseEvent) {
  WebMouseEvent mouse_move = SyntheticWebMouseEventBuilder::Build(
      WebInputEvent::kMouseMove, 10, 10, 0);

  ui::InputPredictor::InputData last_point;
  EXPECT_FALSE(GetPrediction(mouse_move, &last_point));

  HandleEvents(mouse_move);
  EXPECT_EQ(GetPredictorMapSize(), 0);
  EXPECT_TRUE(GetPrediction(mouse_move, &last_point));
  EXPECT_EQ(last_point.pos_x, 10);
  EXPECT_EQ(last_point.pos_y, 10);

  WebMouseEvent mouse_down = SyntheticWebMouseEventBuilder::Build(
      WebInputEvent::kMouseDown, 10, 10, 0);
  HandleEvents(mouse_down);
  EXPECT_FALSE(GetPrediction(mouse_down, &last_point));
}

TEST_F(InputEventPredictionTest, SingleTouchPoint) {
  SyntheticWebTouchEvent touch_event;

  ui::InputPredictor::InputData last_point;

  touch_event.PressPoint(10, 10);
  touch_event.touches[0].pointer_type =
      WebPointerProperties::PointerType::kTouch;
  HandleEvents(touch_event);
  EXPECT_FALSE(GetPrediction(touch_event.touches[0], &last_point));

  touch_event.MovePoint(0, 11, 12);
  HandleEvents(touch_event);
  EXPECT_EQ(GetPredictorMapSize(), 1);
  EXPECT_TRUE(GetPrediction(touch_event.touches[0], &last_point));
  EXPECT_EQ(last_point.pos_x, 11);
  EXPECT_EQ(last_point.pos_y, 12);

  touch_event.ReleasePoint(0);
  HandleEvents(touch_event);
  EXPECT_FALSE(GetPrediction(touch_event.touches[0], &last_point));
}

TEST_F(InputEventPredictionTest, MouseEventTypePen) {
  WebMouseEvent pen_move = SyntheticWebMouseEventBuilder::Build(
      WebInputEvent::kMouseMove, 10, 10, 0,
      WebPointerProperties::PointerType::kPen);

  ui::InputPredictor::InputData last_point;
  EXPECT_FALSE(GetPrediction(pen_move, &last_point));
  HandleEvents(pen_move);
  EXPECT_EQ(GetPredictorMapSize(), 1);
  EXPECT_TRUE(GetPrediction(pen_move, &last_point));
  EXPECT_EQ(last_point.pos_x, 10);
  EXPECT_EQ(last_point.pos_y, 10);

  WebMouseEvent pen_leave = SyntheticWebMouseEventBuilder::Build(
      WebInputEvent::kMouseLeave, 10, 10, 0,
      WebPointerProperties::PointerType::kPen);
  HandleEvents(pen_leave);
  EXPECT_EQ(GetPredictorMapSize(), 0);
  EXPECT_FALSE(GetPrediction(pen_leave, &last_point));
}

TEST_F(InputEventPredictionTest, MultipleTouchPoint) {
  SyntheticWebTouchEvent touch_event;

  // Press and move 1st touch point
  touch_event.PressPoint(10, 10);
  touch_event.MovePoint(0, 11, 12);
  touch_event.touches[0].pointer_type =
      WebPointerProperties::PointerType::kTouch;
  HandleEvents(touch_event);

  // Press 2nd touch point
  touch_event.PressPoint(20, 30);
  touch_event.touches[1].pointer_type = WebPointerProperties::PointerType::kPen;
  HandleEvents(touch_event);
  EXPECT_EQ(GetPredictorMapSize(), 1);

  // Move 2nd touch point
  touch_event.MovePoint(1, 25, 25);
  HandleEvents(touch_event);
  EXPECT_EQ(GetPredictorMapSize(), 2);

  ui::InputPredictor::InputData last_point;
  EXPECT_TRUE(GetPrediction(touch_event.touches[0], &last_point));
  EXPECT_EQ(last_point.pos_x, 11);
  EXPECT_EQ(last_point.pos_y, 12);

  EXPECT_TRUE(GetPrediction(touch_event.touches[1], &last_point));
  EXPECT_EQ(last_point.pos_x, 25);
  EXPECT_EQ(last_point.pos_y, 25);

  touch_event.ReleasePoint(0);
  HandleEvents(touch_event);
  EXPECT_EQ(GetPredictorMapSize(), 1);
}

TEST_F(InputEventPredictionTest, TouchAndStylusResetMousePredictor) {
  WebMouseEvent mouse_move = SyntheticWebMouseEventBuilder::Build(
      WebInputEvent::kMouseMove, 10, 10, 0);
  HandleEvents(mouse_move);
  ui::InputPredictor::InputData last_point;
  EXPECT_TRUE(GetPrediction(mouse_move, &last_point));

  WebMouseEvent pen_move = SyntheticWebMouseEventBuilder::Build(
      WebInputEvent::kMouseMove, 20, 20, 0,
      WebPointerProperties::PointerType::kPen);
  pen_move.id = 1;
  HandleEvents(pen_move);
  EXPECT_TRUE(GetPrediction(pen_move, &last_point));
  EXPECT_FALSE(GetPrediction(mouse_move, &last_point));

  HandleEvents(mouse_move);
  EXPECT_TRUE(GetPrediction(mouse_move, &last_point));

  SyntheticWebTouchEvent touch_event;
  touch_event.PressPoint(10, 10);
  touch_event.touches[0].pointer_type =
      WebPointerProperties::PointerType::kTouch;
  HandleEvents(touch_event);
  touch_event.MovePoint(0, 10, 10);
  HandleEvents(touch_event);
  EXPECT_TRUE(GetPrediction(touch_event.touches[0], &last_point));
  EXPECT_FALSE(GetPrediction(mouse_move, &last_point));
}

// TouchScrollStarted event removes all touch points.
TEST_F(InputEventPredictionTest, TouchScrollStartedRemoveAllTouchPoints) {
  SyntheticWebTouchEvent touch_event;

  // Press 1st & 2nd touch point
  touch_event.PressPoint(10, 10);
  touch_event.touches[0].pointer_type =
      WebPointerProperties::PointerType::kTouch;
  touch_event.PressPoint(20, 20);
  touch_event.touches[1].pointer_type =
      WebPointerProperties::PointerType::kTouch;
  HandleEvents(touch_event);

  // Move 1st & 2nd touch point
  touch_event.MovePoint(0, 15, 18);
  touch_event.MovePoint(1, 25, 27);
  HandleEvents(touch_event);
  EXPECT_EQ(GetPredictorMapSize(), 2);

  touch_event.SetType(WebInputEvent::kTouchScrollStarted);
  HandleEvents(touch_event);
  EXPECT_EQ(GetPredictorMapSize(), 0);
}

}  // namespace content