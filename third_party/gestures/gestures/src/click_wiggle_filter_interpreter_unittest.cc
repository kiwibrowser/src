// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <deque>

#include <gtest/gtest.h>

#include "gestures/include/gestures.h"
#include "gestures/include/macros.h"
#include "gestures/include/click_wiggle_filter_interpreter.h"
#include "gestures/include/unittest_util.h"

using std::deque;

namespace gestures {

class ClickWiggleFilterInterpreterTest : public ::testing::Test {};

class ClickWiggleFilterInterpreterTestInterpreter : public Interpreter {
 public:
  ClickWiggleFilterInterpreterTestInterpreter()
      : Interpreter(NULL, NULL, false),
        expect_warp_(true),
        expected_fingers_(-1) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    if (expected_fingers_ >= 0)
      EXPECT_EQ(expected_fingers_, hwstate->finger_cnt);
    if (hwstate->finger_cnt > 0 && expect_warp_) {
      EXPECT_TRUE(hwstate->fingers[0].flags & GESTURES_FINGER_WARP_X);
      EXPECT_TRUE(hwstate->fingers[0].flags & GESTURES_FINGER_WARP_Y);
    }
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {
    EXPECT_TRUE(false);
  }

  bool expect_warp_;
  short expected_fingers_;
};

TEST(ClickWiggleFilterInterpreterTest, WiggleSuppressTest) {
  ClickWiggleFilterInterpreterTestInterpreter* base_interpreter =
      new ClickWiggleFilterInterpreterTestInterpreter;
  ClickWiggleFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    92,  // right edge
    61,  // bottom edge
    1,  // x pixels/TP width
    1,  // y pixels/TP height
    26,  // x screen DPI
    26,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    0,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&interpreter, &hwprops);

  // These values come from a recording of my finger
  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 38.299999, 0, 43.195655, 32.814815, 1, 0},
    {0, 0, 0, 0, 39.820442, 0, 43.129665, 32.872276, 1, 0},
    {0, 0, 0, 0, 44.924972, 0, 42.881202, 33.077861, 1, 0},
    {0, 0, 0, 0, 52.412372, 0, 42.476348, 33.405296, 1, 0},
    {0, 0, 0, 0, 59.623386, 0, 42.064849, 33.772129, 1, 0},
    {0, 0, 0, 0, 65.317642, 0, 41.741107, 34.157428, 1, 0},
    {0, 0, 0, 0, 69.175155, 0, 41.524814, 34.531333, 1, 0},
    {0, 0, 0, 0, 71.559425, 0, 41.390705, 34.840869, 1, 0},
    {0, 0, 0, 0, 73.018020, 0, 41.294445, 35.082786, 1, 0},
    {0, 0, 0, 0, 73.918144, 0, 41.210456, 35.280235, 1, 0},
    {0, 0, 0, 0, 74.453460, 0, 41.138065, 35.426036, 1, 0},
    {0, 0, 0, 0, 74.585144, 0, 41.084125, 35.506179, 1, 0},
    {0, 0, 0, 0, 74.297470, 0, 41.052356, 35.498870, 1, 0},
    {0, 0, 0, 0, 73.479888, 0, 41.064708, 35.364994, 1, 0},
    {0, 0, 0, 0, 71.686737, 0, 41.178459, 35.072589, 1, 0},
    {0, 0, 0, 0, 68.128448, 0, 41.473480, 34.566291, 1, 0},
    {0, 0, 0, 0, 62.086532, 0, 42.010086, 33.763534, 1, 0},
    {0, 0, 0, 0, 52.739898, 0, 42.745056, 32.644023, 1, 0},
  };
  HardwareState hardware_state[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 1319735240.654559, 1, 1, 1, &finger_states[0], 0, 0, 0, 0 },
    { 1319735240.667746, 1, 1, 1, &finger_states[1], 0, 0, 0, 0 },
    { 1319735240.680153, 1, 1, 1, &finger_states[2], 0, 0, 0, 0 },
    { 1319735240.693717, 1, 1, 1, &finger_states[3], 0, 0, 0, 0 },
    { 1319735240.707821, 1, 1, 1, &finger_states[4], 0, 0, 0, 0 },
    { 1319735240.720633, 1, 1, 1, &finger_states[5], 0, 0, 0, 0 },
    { 1319735240.733183, 1, 1, 1, &finger_states[6], 0, 0, 0, 0 },
    { 1319735240.746131, 1, 1, 1, &finger_states[7], 0, 0, 0, 0 },
    { 1319735240.758622, 1, 1, 1, &finger_states[8], 0, 0, 0, 0 },
    { 1319735240.772690, 1, 1, 1, &finger_states[9], 0, 0, 0, 0 },
    { 1319735240.785556, 1, 1, 1, &finger_states[10], 0, 0, 0, 0 },
    { 1319735240.798524, 1, 1, 1, &finger_states[11], 0, 0, 0, 0 },
    { 1319735240.811093, 1, 1, 1, &finger_states[12], 0, 0, 0, 0 },
    { 1319735240.824775, 1, 1, 1, &finger_states[13], 0, 0, 0, 0 },
    { 1319735240.837738, 0, 1, 1, &finger_states[14], 0, 0, 0, 0 },
    { 1319735240.850482, 0, 1, 1, &finger_states[15], 0, 0, 0, 0 },
    { 1319735240.862749, 0, 1, 1, &finger_states[16], 0, 0, 0, 0 },
    { 1319735240.876571, 0, 1, 1, &finger_states[17], 0, 0, 0, 0 },
    { 1319735240.888128, 0, 0, 0, NULL, 0, 0, 0, 0 }
  };

  for (size_t i = 0; i < arraysize(hardware_state); ++i)
    // Assertions happen in the base interpreter
    wrapper.SyncInterpret(&hardware_state[i], NULL);
}

TEST(ClickWiggleFilterInterpreterTest, OneFingerClickSuppressTest) {
  ClickWiggleFilterInterpreterTestInterpreter* base_interpreter =
      new ClickWiggleFilterInterpreterTestInterpreter;
  ClickWiggleFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    92,  // right edge
    61,  // bottom edge
    1,  // x pixels/TP width
    1,  // y pixels/TP height
    26,  // x screen DPI
    26,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    0,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&interpreter, &hwprops);

  // These values come from a recording of my finger
  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    {0, 0, 0, 0, 38, 0, 43, 45, 1, 0},  // 0
    {0, 0, 0, 0, 37, 0, 43, 48, 1, 0},  // 1
    {0, 0, 0, 0, 38, 0, 43, 49, 1, 0},  // 2
    {0, 0, 0, 0, 38, 0, 43, 49, 1, 0},  // 3 (same as 2)
    {0, 0, 0, 0, 38, 0, 43, 52, 1, 0},  // 4
    {0, 0, 0, 0, 38, 0, 43, 55, 1, 0},  // 5
    {0, 0, 0, 0, 38, 0, 43, 58, 1, 0},  // 6
    {0, 0, 0, 0, 38, 0, 43, 58, 1, 0},  // 7
  };
  HardwareState hardware_state[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 1.0, 1, 1, 1, &finger_states[0], 0, 0, 0, 0 },  // 0
    { 1.1, 1, 1, 1, &finger_states[1], 0, 0, 0, 0 },  // 1
    { 1.11, 1, 1, 1, &finger_states[2], 0, 0, 0, 0 },  // 2
    { 1.25, 1, 1, 1, &finger_states[3], 0, 0, 0, 0 },
    // 3, stable & > Timeout => no warp
    { 1.5, 0, 1, 1, &finger_states[4], 0, 0, 0, 0 },  // 4 button up
    { 1.6, 0, 1, 1, &finger_states[5], 0, 0, 0, 0 },  // 5
    { 1.61, 0, 1, 1, &finger_states[6], 0, 0, 0, 0 },  // 6
    { 1.85, 0, 1, 1, &finger_states[7], 0, 0, 0, 0 },
    // 7, stable & > Timeout => no warp
  };

  interpreter.one_finger_click_wiggle_timeout_.val_ = 0.2;

  for (size_t i = 0; i < arraysize(hardware_state); ++i) {
    // Assertions happen in the base interpreter
    base_interpreter->expect_warp_ = (i != 3 && i != 7);
    wrapper.SyncInterpret(&hardware_state[i], NULL);
  }
}

namespace {
struct ThumbClickTestInput {
  stime_t timestamp_;
  float x_, y_, pressure_;
  short buttons_down_;
};
}  // namespace {}

// This tests uses actual data from a log from Ryan Tabone, where he clicked
// with his thumb and the cursor moved.
TEST(ClickWiggleFilterInterpreter, ThumbClickTest) {
  ClickWiggleFilterInterpreterTestInterpreter* base_interpreter =
      new ClickWiggleFilterInterpreterTestInterpreter;
  ClickWiggleFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    92,  // right edge
    61,  // bottom edge
    1,  // x pixels/TP width
    1,  // y pixels/TP height
    26,  // x screen DPI
    26,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    0,  // is button pad
    0   // has_wheel
  };

  TestInterpreterWrapper wrapper(&interpreter, &hwprops);

  ThumbClickTestInput inputs[] = {
    { 1.089467, 27.83, 21.20, 11.48, 0 },
    { 1.102349, 28.08, 21.20, 17.30, 0 },
    { 1.115390, 28.83, 19.80, 19.24, 0 },
    { 1.128652, 29.91, 18.60, 19.24, 0 },
    { 1.141682, 30.50, 17.30, 19.24, 0 },
    { 1.154569, 31.33, 16.80, 19.24, 1 },
    { 1.168041, 31.91, 16.20, 21.18, 1 },
    { 1.181294, 32.58, 15.90, 09.54, 1 },
  };
  for (size_t i = 0; i < arraysize(inputs); i++) {
    const ThumbClickTestInput& input = inputs[i];
    FingerState fs = {
      0, 0, 0, 0,  // touch/width major/minor
      input.pressure_,
      0,  // orientation
      input.x_, input.y_,
      1,  // tracking id
      0  // flags
    };
    HardwareState hs = {
        input.timestamp_, input.buttons_down_, 1, 1, &fs, 0, 0, 0, 0 };
    wrapper.SyncInterpret(&hs, NULL);
    // Assertions tested in base interpreter
  }
}

// This test makes sure we can do 1-finger movement if the clock goes
// backwards.
TEST(ClickWiggleFilterInterpreter, TimeBackwardsTest) {
  ClickWiggleFilterInterpreterTestInterpreter* base_interpreter =
      new ClickWiggleFilterInterpreterTestInterpreter;
  ClickWiggleFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    92,  // right edge
    61,  // bottom edge
    1,  // x pixels/TP width
    1,  // y pixels/TP height
    26,  // x screen DPI
    26,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    0,  // is button pad
    0   // has_wheel
  };

  TestInterpreterWrapper wrapper(&interpreter, &hwprops);

  const short kInitialId = 1;
  FingerState fs = { 0, 0, 0, 0, 50, 0, 20, 20, kInitialId, 0 };

  HardwareState hs[] = {
    // click
    { 9.00, 1, 1, 1, &fs, 0, 0, 0, 0 },
    { 9.01, 0, 1, 1, &fs, 0, 0, 0, 0 },
    // time goes backwards
    { 1.00, 0, 1, 1, &fs, 0, 0, 0, 0 },
    // long time passes, shouldn't be wobbling anymore
    { 2.01, 0, 1, 1, &fs, 0, 0, 0, 0 },
  };

  for (size_t i = 0; i < arraysize(hs); i++) {
    fs.flags = 0;
    if (hs[i].timestamp < hs[0].timestamp) {
      fs.tracking_id = kInitialId + 1;
    }
    if (i == arraysize(hs) - 1)
      base_interpreter->expect_warp_ = false;
    wrapper.SyncInterpret(&hs[i], NULL);
    if (i == arraysize(hs) - 1)
      EXPECT_EQ(0, fs.flags);
  }
}

struct ThumbClickWiggleWithPalmTestInputs {
  stime_t now;
  int buttons_down;
  // finger data for two fingers:
  float x0, y0, p0;
  unsigned flags0;
  float x1, y1, p1;
  unsigned flags1;
};

// Based on a log from real use by Andrew de los Reyes, where a thumb was
// clicking and wiggling a lot. We would have blocked the wiggle (since it
// was the only finger), but part way through a palm appeared, and we didn't
// realize it was a palm, so we started allowing motion again. This test
// makes sure we don't regress on this log.
TEST(ClickWiggleFilterInterpreter, ThumbClickWiggleWithPalmTest) {
  ClickWiggleFilterInterpreterTestInterpreter* base_interpreter =
      new ClickWiggleFilterInterpreterTestInterpreter;
  ClickWiggleFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  HardwareProperties hwprops = {
    0.000000,  // left edge
    0.000000,  // top edge
    106.666672,  // right edge
    68.000000,  // bottom edge
    1.000000,  // x pixels/TP width
    1.000000,  // y pixels/TP height
    25.400000,  // x screen DPI
    25.400000,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    15,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };

  TestInterpreterWrapper wrapper(&interpreter, &hwprops);

  // Ideally flags will not get renumbered, but just in case.
  const unsigned k8 = GESTURES_FINGER_POSSIBLE_PALM;
  const unsigned k41 = GESTURES_FINGER_POSSIBLE_PALM | GESTURES_FINGER_WARP_X;
  const unsigned k107 = GESTURES_FINGER_POSSIBLE_PALM |
      GESTURES_FINGER_WARP_X | GESTURES_FINGER_WARP_Y;
  const unsigned k16 = GESTURES_FINGER_PALM;
  const unsigned k49 = GESTURES_FINGER_PALM | GESTURES_FINGER_WARP_X;
  const unsigned k115 = GESTURES_FINGER_PALM |
      GESTURES_FINGER_WARP_X | GESTURES_FINGER_WARP_Y;

  ASSERT_EQ(8, k8);
  ASSERT_EQ(41, k41);
  ASSERT_EQ(107, k107);
  ASSERT_EQ(16, k16);
  ASSERT_EQ(49, k49);
  ASSERT_EQ(115, k115);

  ThumbClickWiggleWithPalmTestInputs inputs[] = {
    { 4.5231, 0, 52.0, 59.8, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.5325, 0, 52.0, 59.9, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.5418, 0, 51.6, 59.9, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.5511, 0, 51.5, 59.9, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.5603, 0, 51.5, 59.9, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.5696, 0, 51.4, 59.9, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.5788, 0, 51.2, 59.9, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.5881, 0, 51.1, 59.9, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.5974, 1, 51.1, 59.9, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.6162, 1, 51.0, 59.9, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.6347, 1, 51.0, 59.9, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.6533, 1, 51.0, 60.0, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.6623, 1, 51.0, 60.2, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.6716, 1, 51.0, 60.6, 118.20, k16,   0.0,  0.0,  0.00,    0 },
    { 4.6824, 1, 51.0, 60.9, 118.20, k8,   99.9, 67.2,  3.71,   k8 },
    { 4.6934, 1, 51.9, 61.5, 118.20, k8,   99.9, 67.2,  7.60,   k8 },
    { 4.7042, 1, 53.0, 61.7, 118.20, k8,   99.9, 67.2,  9.54,   k8 },
    { 4.7152, 1, 56.0, 63.4, 118.20, k107, 99.9, 67.2,  9.54,   k8 },
    { 4.7262, 1, 56.4, 63.4, 118.20, k8,  100.4, 67.4, 15.36,  k41 },
    { 4.7372, 0, 57.1, 63.5, 110.43, k8,  100.2, 67.7, 15.36, k107 },
    { 4.7481, 0, 57.8, 64.3,  83.27, k8,  100.2, 67.7, 15.36,   k8 },
    { 4.7591, 0, 60.1, 65.7,  34.76, k41, 100.2, 67.7, 15.36,   k8 },
    { 4.7681, 0,  0.0,  0.0,   0.00, 0,   100.2, 67.7, 15.36,  k49 },
    { 4.7772, 0,  0.0,  0.0,   0.00, 0,   100.1, 67.7, 13.42, k115 },
    { 4.7862, 0,  0.0,  0.0,   0.00, 0,   100.6, 67.7,  9.54,  k49 },
    { 4.7953, 0,  0.0,  0.0,   0.00, 0,   102.2, 67.7,  3.71,  k49 }
  };

  for (size_t i = 0; i < arraysize(inputs); i++) {
    const ThumbClickWiggleWithPalmTestInputs& input = inputs[i];
    FingerState fs[] = {
      { 0, 0, 0, 0, input.p0, 0, input.x0, input.y0, 1, input.flags0 },
      { 0, 0, 0, 0, input.p1, 0, input.x1, input.y1, 2, input.flags1 }
    };
    unsigned short finger_count = (input.p0 == 0.0 || input.p1 == 0.0) ? 1 : 2;
    HardwareState hs = {
      input.now, input.buttons_down, finger_count, finger_count,
      input.p0 == 0.0 ? &fs[1] : &fs[0], 0, 0, 0, 0
    };
    base_interpreter->expect_warp_ = !!input.buttons_down;
    base_interpreter->expected_fingers_ = finger_count;
    wrapper.SyncInterpret(&hs, NULL);
  }
}

}  // namespace gestures
