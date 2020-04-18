// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <deque>
#include <math.h>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "gestures/include/accel_filter_interpreter.h"
#include "gestures/include/gestures.h"
#include "gestures/include/macros.h"
#include "gestures/include/unittest_util.h"
#include "gestures/include/util.h"

using std::deque;
using std::make_pair;
using std::pair;
using std::vector;

namespace gestures {

class AccelFilterInterpreterTest : public ::testing::Test {};

class AccelFilterInterpreterTestInterpreter : public Interpreter {
 public:
  AccelFilterInterpreterTestInterpreter() : Interpreter(NULL, NULL, false) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    if (return_values_.empty())
      return;
    return_value_ = return_values_.front();
    return_values_.pop_front();
    if (return_value_.type == kGestureTypeNull)
      return;
    ProduceGesture(return_value_);
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {
    return SyncInterpret(NULL, NULL);
  }

  Gesture return_value_;
  deque<Gesture> return_values_;
};

TEST(AccelFilterInterpreterTest, SimpleTest) {
  AccelFilterInterpreterTestInterpreter* base_interpreter =
      new AccelFilterInterpreterTestInterpreter;
  AccelFilterInterpreter accel_interpreter(NULL, base_interpreter, NULL);
  TestInterpreterWrapper interpreter(&accel_interpreter);

  accel_interpreter.scroll_x_out_scale_.val_ =
      accel_interpreter.scroll_y_out_scale_.val_ = 1.0;

  float last_move_dx = 0.0;
  float last_move_dy = 0.0;
  float last_scroll_dx = 0.0;
  float last_scroll_dy = 0.0;
  float last_fling_vx = 0.0;
  float last_fling_vy = 0.0;

  for (int i = 1; i <= 5; ++i) {
    accel_interpreter.pointer_sensitivity_.val_ = i;
    accel_interpreter.scroll_sensitivity_.val_ = i;

    base_interpreter->return_values_.push_back(Gesture());  // Null type
    base_interpreter->return_values_.push_back(Gesture(kGestureMove,
                                                       1,  // start time
                                                       1.001,  // end time
                                                       -4,  // dx
                                                       2.8));  // dy
    base_interpreter->return_values_.push_back(Gesture(kGestureScroll,
                                                       2,  // start time
                                                       2.1,  // end time
                                                       4.1,  // dx
                                                       -10.3));  // dy
    base_interpreter->return_values_.push_back(Gesture(kGestureFling,
                                                       3,  // start time
                                                       3.1,  // end time
                                                       100.1,  // vx
                                                       -10.3,  // vy
                                                       0));  // state

    Gesture* out = interpreter.SyncInterpret(NULL, NULL);
    ASSERT_EQ(reinterpret_cast<Gesture*>(NULL), out);
    out = interpreter.SyncInterpret(NULL, NULL);
    ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
    EXPECT_EQ(kGestureTypeMove, out->type);
    if (i == 1) {
      // Expect no acceleration
      EXPECT_FLOAT_EQ(-4.0, out->details.move.dx) << "i = " << i;
      EXPECT_FLOAT_EQ(2.8, out->details.move.dy);
    } else {
      // Expect increasing acceleration
      EXPECT_GT(fabsf(out->details.move.dx), fabsf(last_move_dx));
      EXPECT_GT(fabsf(out->details.move.dy), fabsf(last_move_dy));
    }
    last_move_dx = out->details.move.dx;
    last_move_dy = out->details.move.dy;

    out = interpreter.SyncInterpret(NULL, NULL);
    ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
    EXPECT_EQ(kGestureTypeScroll, out->type);
    if (i == 1) {
      // Expect no acceleration
      EXPECT_FLOAT_EQ(4.1, out->details.scroll.dx);
      EXPECT_FLOAT_EQ(-10.3, out->details.scroll.dy);
    } else if (i > 2) {
      // Expect increasing acceleration
      EXPECT_GT(fabsf(out->details.scroll.dx), fabsf(last_scroll_dx));
      EXPECT_GT(fabsf(out->details.scroll.dy), fabsf(last_scroll_dy));
    }
    last_scroll_dx = out->details.scroll.dx;
    last_scroll_dy = out->details.scroll.dy;
    out = interpreter.SyncInterpret(NULL, NULL);
    ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
    EXPECT_EQ(kGestureTypeFling, out->type);
    if (i == 1) {
      // Expect no acceleration
      EXPECT_FLOAT_EQ(100.1, out->details.fling.vx);
      EXPECT_FLOAT_EQ(-10.3, out->details.fling.vy);
    } else if (i > 2) {
      // Expect increasing acceleration
      EXPECT_GT(fabsf(out->details.fling.vx), fabsf(last_fling_vx));
      EXPECT_GT(fabsf(out->details.fling.vy), fabsf(last_fling_vy));
    }
    last_fling_vx = out->details.fling.vx;
    last_fling_vy = out->details.fling.vy;
  }
}

TEST(AccelFilterInterpreterTest, TinyMoveTest) {
  AccelFilterInterpreterTestInterpreter* base_interpreter =
      new AccelFilterInterpreterTestInterpreter;
  AccelFilterInterpreter accel_interpreter(NULL, base_interpreter, NULL);
  TestInterpreterWrapper interpreter(&accel_interpreter);
  accel_interpreter.scroll_x_out_scale_.val_ =
      accel_interpreter.scroll_y_out_scale_.val_ = 1.0;

  base_interpreter->return_values_.push_back(Gesture(kGestureMove,
                                                     1,  // start time
                                                     2,  // end time
                                                     4,  // dx
                                                     0));  // dy
  base_interpreter->return_values_.push_back(Gesture(kGestureScroll,
                                                     2,  // start time
                                                     3,  // end time
                                                     4,  // dx
                                                     0));  // dy
  base_interpreter->return_values_.push_back(Gesture(kGestureScroll,
                                                     2,  // start time
                                                     3,  // end time
                                                     4,  // dx
                                                     0));  // dy

  Gesture* out = interpreter.SyncInterpret(NULL, NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeMove, out->type);
  EXPECT_GT(fabsf(out->details.move.dx), 2);
  out = interpreter.SyncInterpret(NULL, NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeScroll, out->type);
  EXPECT_GT(fabsf(out->details.scroll.dx), 2);
  float orig_x_scroll = out->details.scroll.dx;
  accel_interpreter.scroll_x_out_scale_.val_ = 2.0;
  out = interpreter.SyncInterpret(NULL, NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeScroll, out->type);
  EXPECT_FLOAT_EQ(orig_x_scroll * accel_interpreter.scroll_x_out_scale_.val_,
                  out->details.scroll.dx);
}

TEST(AccelFilterInterpreterTest, TimingTest) {
  AccelFilterInterpreterTestInterpreter* base_interpreter =
      new AccelFilterInterpreterTestInterpreter;
  AccelFilterInterpreter accel_interpreter(NULL, base_interpreter, NULL);
  TestInterpreterWrapper interpreter(&accel_interpreter);
  accel_interpreter.scroll_x_out_scale_.val_ =
      accel_interpreter.scroll_y_out_scale_.val_ = 1.0;
  accel_interpreter.min_reasonable_dt_.val_ = 0.0;
  accel_interpreter.max_reasonable_dt_.val_ = INFINITY;

  accel_interpreter.pointer_sensitivity_.val_ = 3;  // standard sensitivity
  accel_interpreter.scroll_sensitivity_.val_ = 3;  // standard sensitivity

  float last_dx = 0.0;
  float last_dy = 0.0;

  base_interpreter->return_values_.push_back(Gesture());  // Null type
  base_interpreter->return_values_.push_back(Gesture(kGestureMove,
                                                     1,  // start time
                                                     1.001,  // end time
                                                     -4,  // dx
                                                     2.8));  // dy
  base_interpreter->return_values_.push_back(Gesture(kGestureMove,
                                                     2,  // start time
                                                     3,  // end time
                                                     -4,  // dx
                                                     2.8));  // dy
  base_interpreter->return_values_.push_back(Gesture(kGestureScroll,
                                                     3,  // start time
                                                     3.001,  // end time
                                                     4.1,  // dx
                                                     -10.3));  // dy
  base_interpreter->return_values_.push_back(Gesture(kGestureScroll,
                                                     4,  // start time
                                                     5,  // end time
                                                     4.1,  // dx
                                                     -10.3));  // dy

  Gesture* out = interpreter.SyncInterpret(NULL, NULL);
  ASSERT_EQ(reinterpret_cast<Gesture*>(NULL), out);
  out = interpreter.SyncInterpret(NULL, NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeMove, out->type);
  // Expect less accel for same movement over more time
  last_dx = out->details.move.dx;
  last_dy = out->details.move.dy;
  out = interpreter.SyncInterpret(NULL, NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeMove, out->type);
  EXPECT_GT(fabsf(last_dx), fabsf(out->details.move.dx));
  EXPECT_GT(fabsf(last_dy), fabsf(out->details.move.dy));


  out = interpreter.SyncInterpret(NULL, NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeScroll, out->type);
  // Expect less accel for same movement over more time
  last_dx = out->details.scroll.dx;
  last_dy = out->details.scroll.dy;
  out = interpreter.SyncInterpret(NULL, NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeScroll, out->type);
  EXPECT_GT(fabsf(last_dx), fabsf(out->details.scroll.dx));
  EXPECT_GT(fabsf(last_dy), fabsf(out->details.scroll.dy));
}

TEST(AccelFilterInterpreterTest, CustomAccelTest) {
  AccelFilterInterpreterTestInterpreter* base_interpreter =
      new AccelFilterInterpreterTestInterpreter;
  AccelFilterInterpreter accel_interpreter(NULL, base_interpreter, NULL);
  TestInterpreterWrapper interpreter(&accel_interpreter);
  accel_interpreter.scroll_x_out_scale_.val_ =
      accel_interpreter.scroll_y_out_scale_.val_ = 1.0;
  accel_interpreter.min_reasonable_dt_.val_ = 0.0;
  accel_interpreter.max_reasonable_dt_.val_ = INFINITY;

  // custom sensitivity
  accel_interpreter.use_custom_tp_point_curve_.val_ = 1;
  accel_interpreter.use_custom_tp_scroll_curve_.val_ = 1;
  accel_interpreter.tp_custom_point_[0] =
      AccelFilterInterpreter::CurveSegment(2.0, 0.0, 0.5, 0.0);
  accel_interpreter.tp_custom_point_[1] =
      AccelFilterInterpreter::CurveSegment(3.0, 0.0, 2.0, -3.0);
  accel_interpreter.tp_custom_point_[2] =
      AccelFilterInterpreter::CurveSegment(INFINITY, 0.0, 0.0, 3.0);
  accel_interpreter.tp_custom_scroll_[0] =
      AccelFilterInterpreter::CurveSegment(0.5, 0.0, 2.0, 0.0);
  accel_interpreter.tp_custom_scroll_[1] =
      AccelFilterInterpreter::CurveSegment(1.0, 0.0, 2.0, 0.0);
  accel_interpreter.tp_custom_scroll_[2] =
      AccelFilterInterpreter::CurveSegment(2.0, 0.0, 0.0, 2.0);
  accel_interpreter.tp_custom_scroll_[3] =
      AccelFilterInterpreter::CurveSegment(INFINITY, 0.0, 2.0, -2.0);

  float move_in[]  = { 1.0, 2.5, 3.5, 5.0 };
  float move_out[] = { 0.5, 2.0, 3.0, 3.0 };

  for (size_t i = 0; i < arraysize(move_in); ++i) {
    float dist = move_in[i];
    float expected = move_out[i];
    base_interpreter->return_values_.push_back(Gesture(kGestureMove,
                                                       1,  // start time
                                                       2,  // end time
                                                       dist,  // dx
                                                       0));  // dy
    base_interpreter->return_values_.push_back(Gesture(kGestureMove,
                                                       1,  // start time
                                                       2,  // end time
                                                       0,  // dx
                                                       dist));  // dy
    // half time, half distance = same speed
    base_interpreter->return_values_.push_back(Gesture(kGestureMove,
                                                       1,  // start time
                                                       1.5,  // end time
                                                       dist / 2.0,  // dx
                                                       0));  // dy
    base_interpreter->return_values_.push_back(Gesture(kGestureMove,
                                                       1,  // start time
                                                       1.5,  // end time
                                                       0,  // dx
                                                       dist / 2.0));  // dy

    Gesture* out = interpreter.SyncInterpret(NULL, NULL);
    ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out) << "i=" << i;
    EXPECT_EQ(kGestureTypeMove, out->type) << "i=" << i;
    EXPECT_FLOAT_EQ(expected, out->details.move.dx) << "i=" << i;
    EXPECT_FLOAT_EQ(0, out->details.move.dy) << "i=" << i;

    out = interpreter.SyncInterpret(NULL, NULL);
    ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out) << "i=" << i;
    EXPECT_EQ(kGestureTypeMove, out->type) << "i=" << i;
    EXPECT_FLOAT_EQ(0, out->details.move.dx) << "i=" << i;
    EXPECT_FLOAT_EQ(expected, out->details.move.dy) << "i=" << i;

    out = interpreter.SyncInterpret(NULL, NULL);
    ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out) << "i=" << i;
    EXPECT_EQ(kGestureTypeMove, out->type) << "i=" << i;
    EXPECT_FLOAT_EQ(expected / 2.0, out->details.move.dx) << "i=" << i;
    EXPECT_FLOAT_EQ(0, out->details.move.dy) << "i=" << i;

    out = interpreter.SyncInterpret(NULL, NULL);
    ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out) << "i=" << i;
    EXPECT_EQ(kGestureTypeMove, out->type) << "i=" << i;
    EXPECT_FLOAT_EQ(0, out->details.move.dx) << "i=" << i;
    EXPECT_FLOAT_EQ(expected / 2.0, out->details.move.dy) << "i=" << i;
  }

  float scroll_in[]  = { 0.25, 0.5, 0.75, 1.5, 2.5, 3.0, 3.5 };
  float scroll_out[] = { 0.5,  1.0, 1.5,  2.0, 3.0, 4.0, 5.0 };

  for (size_t i = 0; i < arraysize(scroll_in); ++i) {
    float dist = scroll_in[i];
    float expected = scroll_out[i];
    base_interpreter->return_values_.push_back(Gesture(kGestureScroll,
                                                       1,  // start time
                                                       2,  // end time
                                                       dist,  // dx
                                                       0));  // dy
    base_interpreter->return_values_.push_back(Gesture(kGestureScroll,
                                                       1,  // start time
                                                       2,  // end time
                                                       0,  // dx
                                                       dist));  // dy
    // half time, half distance = same speed
    base_interpreter->return_values_.push_back(Gesture(kGestureScroll,
                                                       1,  // start time
                                                       1.5,  // end time
                                                       dist / 2.0,  // dx
                                                       0));  // dy
    base_interpreter->return_values_.push_back(Gesture(kGestureScroll,
                                                       1,  // start time
                                                       1.5,  // end time
                                                       0,  // dx
                                                       dist / 2.0));  // dy

    Gesture* out = interpreter.SyncInterpret(NULL, NULL);
    ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out) << "i=" << i;
    EXPECT_EQ(kGestureTypeScroll, out->type) << "i=" << i;
    EXPECT_FLOAT_EQ(expected, out->details.scroll.dx) << "i=" << i;
    EXPECT_FLOAT_EQ(0, out->details.scroll.dy) << "i=" << i;

    out = interpreter.SyncInterpret(NULL, NULL);
    ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out) << "i=" << i;
    EXPECT_EQ(kGestureTypeScroll, out->type) << "i=" << i;
    EXPECT_FLOAT_EQ(0, out->details.scroll.dx) << "i=" << i;
    EXPECT_FLOAT_EQ(expected, out->details.scroll.dy) << "i=" << i;

    out = interpreter.SyncInterpret(NULL, NULL);
    ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out) << "i=" << i;
    EXPECT_EQ(kGestureTypeScroll, out->type) << "i=" << i;
    EXPECT_FLOAT_EQ(expected / 2.0, out->details.scroll.dx) << "i=" << i;
    EXPECT_FLOAT_EQ(0, out->details.scroll.dy) << "i=" << i;

    out = interpreter.SyncInterpret(NULL, NULL);
    ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out) << "i=" << i;
    EXPECT_EQ(kGestureTypeScroll, out->type) << "i=" << i;
    EXPECT_FLOAT_EQ(0, out->details.scroll.dx) << "i=" << i;
    EXPECT_FLOAT_EQ(expected / 2.0, out->details.scroll.dy) << "i=" << i;
  }
}

}  // namespace gestures
