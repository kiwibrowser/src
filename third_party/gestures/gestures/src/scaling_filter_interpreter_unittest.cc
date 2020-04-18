// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <deque>
#include <math.h>
#include <memory>
#include <vector>
#include <utility>

#include <gtest/gtest.h>

#include "gestures/include/gestures.h"
#include "gestures/include/scaling_filter_interpreter.h"
#include "gestures/include/unittest_util.h"
#include "gestures/include/util.h"

using std::deque;
using std::make_pair;
using std::pair;

namespace gestures {

class ScalingFilterInterpreterTest : public ::testing::Test {};

class ScalingFilterInterpreterTestInterpreter : public Interpreter {
 public:
  ScalingFilterInterpreterTestInterpreter()
      : Interpreter(NULL, NULL, false), initialize_called_(false) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    if (!expected_coordinates_.empty()) {
      std::vector<pair<float, float> >& expected =
          expected_coordinates_.front();
      for (unsigned short i = 0; i < hwstate->finger_cnt; i++) {
        EXPECT_FLOAT_EQ(expected[i].first, hwstate->fingers[i].position_x)
            << "i = " << i;
        EXPECT_FLOAT_EQ(expected[i].second, hwstate->fingers[i].position_y)
            << "i = " << i;
      }
      expected_coordinates_.pop_front();
    }
    if (!expected_orientation_.empty()) {
      const std::vector<float>& expected = expected_orientation_.front();
      EXPECT_EQ(expected.size(), hwstate->finger_cnt);
      for (size_t i = 0; i < hwstate->finger_cnt; i++)
        EXPECT_FLOAT_EQ(expected[i], hwstate->fingers[i].orientation)
            << "i=" << i;
      expected_orientation_.pop_front();
    }
    if (!expected_touch_major_.empty()) {
      const std::vector<float>& expected = expected_touch_major_.front();
      EXPECT_EQ(expected.size(), hwstate->finger_cnt);
      for (size_t i = 0; i < hwstate->finger_cnt; i++)
        EXPECT_FLOAT_EQ(expected[i], hwstate->fingers[i].touch_major)
            << "i=" << i;
      expected_touch_major_.pop_front();
    }
    if (!expected_touch_minor_.empty()) {
      const std::vector<float>& expected = expected_touch_minor_.front();
      EXPECT_EQ(expected.size(), hwstate->finger_cnt);
      for (size_t i = 0; i < hwstate->finger_cnt; i++)
        EXPECT_FLOAT_EQ(expected[i], hwstate->fingers[i].touch_minor)
            << "i=" << i;
      expected_touch_minor_.pop_front();
    }
    if (!expected_pressures_.empty() && hwstate->finger_cnt > 0) {
      EXPECT_FLOAT_EQ(expected_pressures_.front(),
                      hwstate->fingers[0].pressure);
      expected_pressures_.pop_front();
    } else {
      // Test if the low pressure event is dropped
      EXPECT_EQ(expected_finger_cnt_.front(), hwstate->finger_cnt);
      expected_finger_cnt_.pop_front();
      EXPECT_EQ(expected_touch_cnt_.front(), hwstate->touch_cnt);
      expected_touch_cnt_.pop_front();
    }
    if (return_values_.empty())
      return;
    return_value_ = return_values_.front();
    return_values_.pop_front();
    if (return_value_.type == kGestureTypeNull)
      return;
    ProduceGesture(return_value_);
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {
    EXPECT_TRUE(false);
  }

  virtual void Initialize(const HardwareProperties* hw_props,
                          Metrics* metrics,
                          MetricsProperties* mprops,
                          GestureConsumer* consumer) {
    EXPECT_FLOAT_EQ(expected_hwprops_.left, hw_props->left);
    EXPECT_FLOAT_EQ(expected_hwprops_.top, hw_props->top);
    EXPECT_FLOAT_EQ(expected_hwprops_.right, hw_props->right);
    EXPECT_FLOAT_EQ(expected_hwprops_.bottom, hw_props->bottom);
    EXPECT_FLOAT_EQ(expected_hwprops_.res_x, hw_props->res_x);
    EXPECT_FLOAT_EQ(expected_hwprops_.res_y, hw_props->res_y);
    EXPECT_FLOAT_EQ(expected_hwprops_.screen_x_dpi, hw_props->screen_x_dpi);
    EXPECT_FLOAT_EQ(expected_hwprops_.screen_y_dpi, hw_props->screen_y_dpi);
    EXPECT_FLOAT_EQ(expected_hwprops_.orientation_minimum,
                    hw_props->orientation_minimum);
    EXPECT_FLOAT_EQ(expected_hwprops_.orientation_maximum,
                    hw_props->orientation_maximum);
    EXPECT_EQ(expected_hwprops_.max_finger_cnt, hw_props->max_finger_cnt);
    EXPECT_EQ(expected_hwprops_.max_touch_cnt, hw_props->max_touch_cnt);
    EXPECT_EQ(expected_hwprops_.supports_t5r2, hw_props->supports_t5r2);
    EXPECT_EQ(expected_hwprops_.support_semi_mt, hw_props->support_semi_mt);
    EXPECT_EQ(expected_hwprops_.is_button_pad, hw_props->is_button_pad);
    initialize_called_ = true;
    Interpreter::Initialize(hw_props, metrics, mprops, consumer);
  };

  Gesture return_value_;
  deque<Gesture> return_values_;
  deque<std::vector<pair<float, float> > > expected_coordinates_;
  deque<std::vector<float> > expected_orientation_;
  deque<std::vector<float> > expected_touch_major_;
  deque<std::vector<float> > expected_touch_minor_;
  deque<float> expected_pressures_;
  deque<int> expected_finger_cnt_;
  deque<int> expected_touch_cnt_;
  HardwareProperties expected_hwprops_;
  bool initialize_called_;
};

TEST(ScalingFilterInterpreterTest, SimpleTest) {
  ScalingFilterInterpreterTestInterpreter* base_interpreter =
      new ScalingFilterInterpreterTestInterpreter;
  ScalingFilterInterpreter interpreter(NULL, base_interpreter, NULL,
                                       GESTURES_DEVCLASS_TOUCHPAD);
  HardwareProperties initial_hwprops = {
    133, 728, 10279, 5822,  // left, top, right, bottom
    (10279.0 - 133.0) / 100.0,  // x res (pixels/mm)
    (5822.0 - 728.0) / 60,  // y res (pixels/mm)
    133, 133,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch
    0, 0, 0, 0  //t5r2, semi, button pad
  };
  HardwareProperties expected_hwprops = {
    0, 0, 100, 60,  // left, top, right, bottom
    1.0, 1.0, 25.4, 25.4, // x res, y res, x DPI, y DPI
    -M_PI_4,  // orientation minimum (1 tick above X-axis)
    M_PI_2,   // orientation maximum
    2, 5, 0, 0, 0, 0  // max_fingers, max_touch, t5r2, semi_mt,
  };
  base_interpreter->expected_hwprops_ = expected_hwprops;

  TestInterpreterWrapper wrapper(&interpreter, &initial_hwprops);
  EXPECT_TRUE(base_interpreter->initialize_called_);
  const float kPressureScale = 2.0;
  const float kPressureTranslate = 3.0;
  const float kPressureThreshold = 10.0;
  interpreter.pressure_scale_.val_ = kPressureScale;
  interpreter.pressure_translate_.val_ = kPressureTranslate;
  const float kTpYBias = -2.8;
  interpreter.tp_y_bias_.val_ = kTpYBias;

  FingerState fs[] = {
    { 1, 0, 0, 0, 1, 0, 150, 4000, 1, 0 },
    { 0, 0, 0, 0, 2, 0, 550, 2000, 1, 0 },
    { 0, 0, 0, 0, 3, 0, 250, 3000, 1, 0 },
    { 0, 0, 0, 0, 3, 0, 250, 3000, 1, 0 }
  };
  HardwareState hs[] = {
    { 10000, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 54000, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    { 98000, 0, 1, 1, &fs[2], 0, 0, 0, 0 },
    { 99000, 0, 1, 1, &fs[3], 0, 0, 0, 0 },
  };

  // Set up expected translated coordinates
  base_interpreter->expected_coordinates_.push_back(
      std::vector<pair<float, float> >(1, make_pair(
          static_cast<float>(100.0 * (150.0 - 133.0) / (10279.0 - 133.0)),
          static_cast<float>(60.0 * (4000.0 - 728.0) / (5822.0 - 728.0)))));
  base_interpreter->expected_coordinates_.push_back(
      std::vector<pair<float, float> >(1, make_pair(
          static_cast<float>(100.0 * (550.0 - 133.0) / (10279.0 - 133.0)),
          static_cast<float>(60.0 * (2000.0 - 728.0) / (5822.0 - 728.0)))));
  base_interpreter->expected_coordinates_.push_back(
      std::vector<pair<float, float> >(1, make_pair(
          static_cast<float>(100.0 * (250.0 - 133.0) / (10279.0 - 133.0)),
          static_cast<float>(60.0 * (3000.0 - 728.0) / (5822.0 - 728.0)))));
  base_interpreter->expected_coordinates_.push_back(
      std::vector<pair<float, float> >(1, make_pair(
          static_cast<float>(100.0 * (250.0 - 133.0) / (10279.0 - 133.0)),
          static_cast<float>(60.0 * (3000.0 - 728.0) / (5822.0 - 728.0)))));

  base_interpreter->expected_pressures_.push_back(
      fs[0].pressure * kPressureScale + kPressureTranslate);
  base_interpreter->expected_pressures_.push_back(
      fs[1].pressure * kPressureScale + kPressureTranslate);
  base_interpreter->expected_pressures_.push_back(
      fs[2].pressure * kPressureScale + kPressureTranslate);
  base_interpreter->expected_pressures_.push_back(
      fs[3].pressure * kPressureScale + kPressureTranslate);

  base_interpreter->expected_touch_major_.push_back(
      std::vector<float>(1, interpreter.tp_y_scale_ *
                       (fs[0].touch_major - kTpYBias)));

  // Set up gestures to return
  base_interpreter->return_values_.push_back(Gesture());  // Null type
  base_interpreter->return_values_.push_back(Gesture(kGestureMove,
                                                     0,  // start time
                                                     0,  // end time
                                                     -4,  // dx
                                                     2.8));  // dy
  base_interpreter->return_values_.push_back(Gesture(kGestureScroll,
                                                     0,  // start time
                                                     0,  // end time
                                                     4.1,  // dx
                                                     -10.3));  // dy
  base_interpreter->return_values_.push_back(Gesture(kGestureFling,
                                                     0,  // start time
                                                     0,  // end time
                                                     201.8,  // dx
                                                     -112.4,  // dy
                                                     GESTURES_FLING_START));
  base_interpreter->return_values_.push_back(Gesture());  // Null type

  Gesture* out = wrapper.SyncInterpret(&hs[0], NULL);
  ASSERT_EQ(reinterpret_cast<Gesture*>(NULL), out);
  out = wrapper.SyncInterpret(&hs[1], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeMove, out->type);
  EXPECT_FLOAT_EQ(-4.0 * 133.0 / 25.4, out->details.move.dx);
  EXPECT_FLOAT_EQ(2.8 * 133.0 / 25.4, out->details.move.dy);
  out = wrapper.SyncInterpret(&hs[2], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeScroll, out->type);
  EXPECT_FLOAT_EQ(-4.1 * 133.0 / 25.4, out->details.scroll.dx);
  EXPECT_FLOAT_EQ(10.3 * 133.0 / 25.4, out->details.scroll.dy);
  out = wrapper.SyncInterpret(&hs[3], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeFling, out->type);
  EXPECT_FLOAT_EQ(-201.8 * 133.0 / 25.4, out->details.fling.vx);
  EXPECT_FLOAT_EQ(112.4 * 133.0 / 25.4, out->details.fling.vy);
  EXPECT_EQ(GESTURES_FLING_START, out->details.fling.fling_state);

  // Test if we will drop the low pressure event.
  FingerState fs2[] = {
    { 0, 0, 0, 0, 1, 0, 150, 4000, 2, 0 },
    { 0, 0, 0, 0, 4, 0, 550, 2000, 2, 0 },
    { 0, 0, 0, 0, 1, 0, 560, 2000, 2, 0 },
  };
  HardwareState hs2[] = {
    { 110000, 0, 1, 2, &fs2[0], 0, 0, 0, 0 },
    { 154000, 0, 1, 1, &fs2[1], 0, 0, 0, 0 },
    { 184000, 0, 1, 0, &fs2[2], 0, 0, 0, 0 }
  };
  interpreter.pressure_threshold_.val_ = kPressureThreshold;
  base_interpreter->expected_finger_cnt_.push_back(0);
  base_interpreter->expected_touch_cnt_.push_back(1);
  out = wrapper.SyncInterpret(&hs2[0], NULL);

  base_interpreter->expected_pressures_.push_back(
      fs2[1].pressure * kPressureScale + kPressureTranslate);
  out = wrapper.SyncInterpret(&hs2[1], NULL);

  base_interpreter->expected_finger_cnt_.push_back(0);
  base_interpreter->expected_touch_cnt_.push_back(0);
  out = wrapper.SyncInterpret(&hs2[2], NULL);
}

static void RunTouchMajorAndMinorTest(
    ScalingFilterInterpreterTestInterpreter* base_interpreter,
    ScalingFilterInterpreter* interpreter,
    HardwareProperties *hwprops,
    HardwareProperties *expected_hwprops,
    FingerState *fs,
    size_t n_fs,
    float e_x,
    float e_y) {

  const float r_x_2 = 1.0 / hwprops->res_x / hwprops->res_x;
  const float r_y_2 = 1.0 / hwprops->res_y / hwprops->res_y;

  float orientation, touch_major, touch_minor, pressure;

  std::unique_ptr<bool[]> has_zero_area(new bool[n_fs]);

  for (size_t i = 0; i < n_fs; i++) {
    bool no_orientation = hwprops->orientation_maximum == 0;
    float cos_2, sin_2, touch_major_bias, touch_minor_bias;
    if (no_orientation)
      orientation = 0;
    else
      orientation = M_PI * fs[i].orientation /
          (hwprops->orientation_maximum - hwprops->orientation_minimum + 1);
    cos_2 = cosf(orientation) * cosf(orientation);
    sin_2 = sinf(orientation) * sinf(orientation);
    touch_major_bias = e_x * sin_2 + e_y * cos_2;
    touch_minor_bias = e_x * cos_2 + e_y * sin_2;
    if (fs[i].touch_major)
      touch_major = fabsf(fs[i].touch_major - touch_major_bias) *
                    sqrtf(r_x_2 * sin_2 + r_y_2 * cos_2);
    else
      touch_major = 0.0;
    if (fs[i].touch_minor)
      touch_minor = fabsf(fs[i].touch_minor - touch_minor_bias) *
                    sqrtf(r_x_2 * cos_2 + r_y_2 * sin_2);
    else
      touch_minor = 0.0;
    if (!no_orientation && touch_major < touch_minor) {
      std::swap(touch_major, touch_minor);
      if (orientation > 0.0)
        orientation -= M_PI_2;
      else
        orientation += M_PI_2;
    }
    if (touch_major && touch_minor)
      pressure = M_PI_4 * touch_major * touch_minor;
    else if (touch_major)
      pressure = M_PI_4 * touch_major * touch_major;
    else
      pressure = 0;

    has_zero_area[i] = pressure == 0.0;

    pressure = std::max(pressure , 1.0f);

    if (has_zero_area[i]) {
      base_interpreter->expected_orientation_.push_back(
          std::vector<float>(0));
      base_interpreter->expected_touch_major_.push_back(
          std::vector<float>(0));
      base_interpreter->expected_touch_minor_.push_back(
          std::vector<float>(0));
      base_interpreter->expected_finger_cnt_.push_back(0);
      base_interpreter->expected_touch_cnt_.push_back(0);
    } else {
      base_interpreter->expected_orientation_.push_back(
          std::vector<float>(1, orientation));
      base_interpreter->expected_touch_major_.push_back(
          std::vector<float>(1, touch_major));
      base_interpreter->expected_touch_minor_.push_back(
          std::vector<float>(1, touch_minor));
      base_interpreter->expected_pressures_.push_back(pressure);
    }
  }

  base_interpreter->expected_hwprops_ = *expected_hwprops;
  interpreter->Initialize(hwprops, NULL, NULL, NULL);
  EXPECT_TRUE(base_interpreter->initialize_called_);

  for (size_t i = 0; i < n_fs; i++) {
    HardwareState hs;
    memset(&hs, 0x0, sizeof(hs));
    hs.timestamp = (i + 1) * 1000;
    if (has_zero_area[i]) {
      hs.finger_cnt = 0;
      hs.touch_cnt = 0;
    } else {
      hs.finger_cnt = 1;
      hs.touch_cnt = 1;
    }
    hs.fingers = fs + i;
    interpreter->SyncInterpret(&hs, NULL);
  }

  // Tear down state
  base_interpreter->initialize_called_ = false;
}

TEST(ScalingFilterInterpreterTest, TouchMajorAndMinorTest) {
  ScalingFilterInterpreterTestInterpreter* base_interpreter =
      new ScalingFilterInterpreterTestInterpreter;
  ScalingFilterInterpreter interpreter(NULL, base_interpreter, NULL,
                                       GESTURES_DEVCLASS_TOUCHPAD);

  const float e_x = 17;
  const float e_y = 71;
  const bool kFilterLowPressure = 1;

  interpreter.surface_area_from_pressure_.val_ = false;
  interpreter.filter_low_pressure_.val_ = kFilterLowPressure;
  interpreter.tp_x_bias_.val_ = e_x;
  interpreter.tp_y_bias_.val_ = e_y;

  HardwareProperties hwprops = {
    0, 0, 500, 1000,  // left, top, right, bottom
    5,  // x res (pixels/mm)
    10,  // y res (pixels/mm)
    133, 133,  // scrn DPI X, Y
    -31,  // orientation minimum
    32,   // orientation maximum
    2, 5,  // max fingers, max_touch
    0, 0, 0, 1  //t5r2, semi, button pad
  };
  HardwareProperties expected_hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    1.0, 1.0, 25.4, 25.4, // x res, y res, x DPI, y DPI
    -M_PI * 31 / 64,  // orientation minimum (1 tick above X-axis)
    M_PI_2,   // orientation maximum
    2, 5, 0, 0, 0, 1  // max_fingers, max_touch, t5r2, semi_mt,
  };

  // Test 1: Touch major and touch minor scaling with orientation
  // range [-31, 32].

  hwprops.orientation_minimum = -31;
  hwprops.orientation_maximum = 32;
  expected_hwprops.orientation_minimum =
      M_PI * hwprops.orientation_minimum /
      (hwprops.orientation_maximum - hwprops.orientation_minimum + 1);
  expected_hwprops.orientation_maximum =
      M_PI * hwprops.orientation_maximum /
      (hwprops.orientation_maximum - hwprops.orientation_minimum + 1);

  FingerState test_1_fs[] = {
    {  0.0,  0.0, 0, 0, 0,   0.0, 0, 0, 1, 0 },

    { 79.0, 99.0, 0, 0, 0,  16.0, 0, 0, 1, 0 },

    { 79.0, 31.0, 0, 0, 0, -16.0, 0, 0, 1, 0 },
    { 79.0, 31.0, 0, 0, 0,   0.0, 0, 0, 1, 0 },
    { 79.0, 31.0, 0, 0, 0,  16.0, 0, 0, 1, 0 },
    { 79.0, 31.0, 0, 0, 0,  32.0, 0, 0, 1, 0 },

    { 79.0,  0.0, 0, 0, 0, -16.0, 0, 0, 1, 0 },
    { 79.0,  0.0, 0, 0, 0,   0.0, 0, 0, 1, 0 },
    { 79.0,  0.0, 0, 0, 0,  16.0, 0, 0, 1, 0 },
    { 79.0,  0.0, 0, 0, 0,  32.0, 0, 0, 1, 0 },
  };

  RunTouchMajorAndMinorTest(base_interpreter,
                            &interpreter,
                            &hwprops,
                            &expected_hwprops,
                            test_1_fs,
                            arraysize(test_1_fs),
                            e_x,
                            e_y);

  // Test 2: Touch major and touch minor scaling with orientation
  // range [0, 1].

  hwprops.orientation_minimum = 0;
  hwprops.orientation_maximum = 1;
  expected_hwprops.orientation_minimum = 0;
  expected_hwprops.orientation_maximum = M_PI_2;

  FingerState test_2_fs[] = {
    {  0.0,  0.0, 0, 0, 0, 0.0, 0, 0, 1, 0 },

    { 79.0, 31.0, 0, 0, 0, 0.0, 0, 0, 1, 0 },
    { 79.0, 31.0, 0, 0, 0, 1.0, 0, 0, 1, 0 },

    { 79.0,  0.0, 0, 0, 0, 0.0, 0, 0, 1, 0 },
    { 79.0,  0.0, 0, 0, 0, 1.0, 0, 0, 1, 0 },
  };

  RunTouchMajorAndMinorTest(base_interpreter,
                            &interpreter,
                            &hwprops,
                            &expected_hwprops,
                            test_2_fs,
                            arraysize(test_2_fs),
                            e_x,
                            e_y);

  // Test 3: Touch major and touch minor scaling with no orientation
  // provided.

  hwprops.orientation_minimum = 0;
  hwprops.orientation_maximum = 0;
  expected_hwprops.orientation_minimum = 0;
  expected_hwprops.orientation_maximum = 0;

  FingerState test_3_fs[] = {
    {  0.0,  0.0, 0, 0, 0, 0.0, 0, 0, 1, 0 },

    { 79.0, 31.0, 0, 0, 0, 0.0, 0, 0, 1, 0 },

    { 79.0,  0.0, 0, 0, 0, 0.0, 0, 0, 1, 0 },
  };

  RunTouchMajorAndMinorTest(base_interpreter,
                            &interpreter,
                            &hwprops,
                            &expected_hwprops,
                            test_3_fs,
                            arraysize(test_3_fs),
                            e_x,
                            e_y);
}

}  // namespace gestures
