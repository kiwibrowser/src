// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <deque>
#include <math.h>
#include <stdio.h>
#include <vector>
#include <utility>

#include <gtest/gtest.h>

#include "gestures/include/gestures.h"
#include "gestures/include/palm_classifying_filter_interpreter.h"
#include "gestures/include/unittest_util.h"

using std::deque;
using std::make_pair;
using std::pair;
using std::vector;

namespace gestures {

class PalmClassifyingFilterInterpreterTest : public ::testing::Test {};

class PalmClassifyingFilterInterpreterTestInterpreter : public Interpreter {
 public:
  PalmClassifyingFilterInterpreterTestInterpreter()
      : Interpreter(NULL, NULL, false),
        expected_flags_(0) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    if (hwstate->finger_cnt > 0) {
      EXPECT_EQ(expected_flags_, hwstate->fingers[0].flags);
    }
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {
    EXPECT_TRUE(false);
  }

  unsigned expected_flags_;
};

TEST(PalmClassifyingFilterInterpreterTest, PalmTest) {
  PalmClassifyingFilterInterpreter pci(NULL, NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    1000,  // right edge
    1000,  // bottom edge
    500,  // x pixels/TP width
    500,  // y pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0  // has_wheel
  };
  TestInterpreterWrapper wrapper(&pci, &hwprops);

  const float kBig = pci.palm_pressure_.val_ + 1;  // big (palm) pressure
  const float kSml = pci.palm_pressure_.val_ - 1;  // low pressure

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, kSml, 0, 600, 500, 1, 0},
    {0, 0, 0, 0, kSml, 0, 500, 500, 2, 0},

    {0, 0, 0, 0, kSml, 0, 600, 500, 1, 0},
    {0, 0, 0, 0, kBig, 0, 500, 500, 2, 0},

    {0, 0, 0, 0, kSml, 0, 600, 500, 1, 0},
    {0, 0, 0, 0, kSml, 0, 500, 500, 2, 0},

    {0, 0, 0, 0, kSml, 0, 600, 500, 3, 0},
    {0, 0, 0, 0, kBig, 0, 500, 500, 4, 0},

    {0, 0, 0, 0, kSml, 0, 600, 500, 3, 0},
    {0, 0, 0, 0, kSml, 0, 500, 500, 4, 0}
  };
  HardwareState hardware_state[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 200000, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 200001, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 },
    { 200002, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 },
    { 200003, 0, 2, 2, &finger_states[6], 0, 0, 0, 0 },
    { 200004, 0, 2, 2, &finger_states[8], 0, 0, 0, 0 },
  };

  for (size_t i = 0; i < 5; ++i) {
    wrapper.SyncInterpret(&hardware_state[i], NULL);
    switch (i) {
      case 0:
        EXPECT_TRUE(SetContainsValue(pci.pointing_, 1));
        EXPECT_FALSE(SetContainsValue(pci.palm_, 1));
        EXPECT_TRUE(SetContainsValue(pci.pointing_, 2));
        EXPECT_FALSE(SetContainsValue(pci.palm_, 2));
        break;
      case 1:  // fallthrough
      case 2:
        EXPECT_TRUE(SetContainsValue(pci.pointing_, 1));
        EXPECT_FALSE(SetContainsValue(pci.palm_, 1));
        EXPECT_FALSE(SetContainsValue(pci.pointing_, 2));
        EXPECT_TRUE(SetContainsValue(pci.palm_, 2));
        break;
      case 3:  // fallthrough
      case 4:
        EXPECT_TRUE(SetContainsValue(pci.pointing_, 3)) << "i=" << i;
        EXPECT_FALSE(SetContainsValue(pci.palm_, 3));
        EXPECT_FALSE(SetContainsValue(pci.pointing_, 4));
        EXPECT_TRUE(SetContainsValue(pci.palm_, 4));
        break;
    }
  }
}

TEST(PalmClassifyingFilterInterpreterTest, StationaryPalmTest) {
  PalmClassifyingFilterInterpreter pci(NULL, NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // x pixels/TP width
    1,  // y pixels/TP height
    1,  // x screen DPI
    1,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    5,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0  // has_wheel
  };
  TestInterpreterWrapper wrapper(&pci, &hwprops);

  const float kPr = pci.palm_pressure_.val_ / 2;

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    // Palm is id 1, finger id 2
    {0, 0, 0, 0, kPr, 0,  0, 40, 1, 0},
    {0, 0, 0, 0, kPr, 0, 60, 37, 2, 0},

    {0, 0, 0, 0, kPr, 0,  0, 40, 1, 0},
    {0, 0, 0, 0, kPr, 0, 60, 40, 2, 0},

    {0, 0, 0, 0, kPr, 0,  0, 40, 1, 0},
    {0, 0, 0, 0, kPr, 0, 60, 43, 2, 0},
  };
  HardwareState hardware_state[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 0.00, 0, 1, 1, &finger_states[0], 0, 0, 0, 0 },
    { 4.00, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 5.00, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 },
    { 5.01, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 },
  };

  for (size_t i = 0; i < arraysize(hardware_state); ++i) {
    wrapper.SyncInterpret(&hardware_state[i], NULL);
    if (i > 0) {
      // We expect after the second input frame is processed that the palm
      // is classified
      EXPECT_FALSE(SetContainsValue(pci.pointing_, 1));
      EXPECT_TRUE(SetContainsValue(pci.palm_, 1));
    }
    if (hardware_state[i].finger_cnt > 1)
      EXPECT_TRUE(SetContainsValue(pci.pointing_, 2)) << "i=" << i;
  }
}

TEST(PalmClassifyingFilterInterpreterTest, PalmAtEdgeTest) {
  PalmClassifyingFilterInterpreterTestInterpreter* base_interpreter = NULL;
  std::unique_ptr<PalmClassifyingFilterInterpreter> pci(
      new PalmClassifyingFilterInterpreter(NULL, NULL, NULL));
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // x pixels/mm
    1,  // y pixels/mm
    1,  // x screen px/mm
    1,  // y screen px/mm
    -1,  // orientation minimum
    2,   // orientation maximum
    5,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0  // has_wheel
  };
  TestInterpreterWrapper wrapper(pci.get(), &hwprops);

  const float kBig = pci->palm_pressure_.val_ + 1.0;  // palm pressure
  const float kSml = pci->palm_pressure_.val_ - 1.0;  // small, low pressure
  const float kMid = pci->palm_pressure_.val_ / 2.0;
  const float kMidWidth =
      (pci->palm_edge_min_width_.val_ + pci->palm_edge_width_.val_) / 2.0;

  const float kBigMove = pci->palm_pointing_min_dist_.val_ + 1.0;
  const float kSmallMove = pci->palm_pointing_min_dist_.val_ - 1.0;

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    // small contact with small movement in edge
    {0, 0, 0, 0, kSml, 0, 1, 40, 1, 0},
    {0, 0, 0, 0, kSml, 0, 1, 40 + kSmallMove, 1, 0},
    // small contact movement in middle
    {0, 0, 0, 0, kSml, 0, 50, 40, 1, 0},
    {0, 0, 0, 0, kSml, 0, 50, 50, 1, 0},
    // large contact movment in middle
    {0, 0, 0, 0, kBig, 0, 50, 40, 1, 0},
    {0, 0, 0, 0, kBig, 0, 50, 50, 1, 0},
    // under mid-pressure contact move at mid-width
    {0, 0, 0, 0, kMid - 1.0f, 0, kMidWidth, 40, 1, 0},
    {0, 0, 0, 0, kMid - 1.0f, 0, kMidWidth, 50, 1, 0},
    // over mid-pressure contact move at mid-width with small movement
    {0, 0, 0, 0, kMid + 1.0f, 0, kMidWidth, 40, 1, 0},
    {0, 0, 0, 0, kMid + 1.0f, 0, kMidWidth, 40 + kSmallMove, 1, 0},
    // small contact with large movement in edge
    {0, 0, 0, 0, kSml, 0, 1, 40, 1, 0},
    {0, 0, 0, 0, kSml, 0, 1, 40 + kBigMove, 1, 0},
    // over mid-pressure contact move at mid-width with large movement
    {0, 0, 0, 0, kMid + 1.0f, 0, kMidWidth, 40, 1, 0},
    {0, 0, 0, 0, kMid + 1.0f, 0, kMidWidth, 40 + kBigMove, 1, 0},
  };
  HardwareState hardware_state[] = {
    // time, buttons, finger count, touch count, finger states pointer
    // slow movement at edge with small movement
    { 0.0, 0, 1, 1, &finger_states[0], 0, 0, 0, 0 },
    { 1.0, 0, 1, 1, &finger_states[1], 0, 0, 0, 0 },
    // slow small contact movement in middle
    { 0.0, 0, 1, 1, &finger_states[2], 0, 0, 0, 0 },
    { 1.0, 0, 1, 1, &finger_states[3], 0, 0, 0, 0 },
    // slow large contact movement in middle
    { 0.0, 0, 1, 1, &finger_states[4], 0, 0, 0, 0 },
    { 1.0, 0, 1, 1, &finger_states[5], 0, 0, 0, 0 },
    // under mid-pressure at mid-width
    { 0.0, 0, 1, 1, &finger_states[6], 0, 0, 0, 0 },
    { 1.0, 0, 1, 1, &finger_states[7], 0, 0, 0, 0 },
    // over mid-pressure at mid-width
    { 0.0, 0, 1, 1, &finger_states[8], 0, 0, 0, 0 },
    { 1.0, 0, 1, 1, &finger_states[9], 0, 0, 0, 0 },
    // large movement at edge
    { 0.0, 0, 1, 1, &finger_states[10], 0, 0, 0, 0 },
    { 1.0, 0, 1, 1, &finger_states[11], 0, 0, 0, 0 },
    // over mid-pressure at mid-width with large movement
    { 0.0, 0, 1, 1, &finger_states[12], 0, 0, 0, 0 },
    { 1.0, 0, 1, 1, &finger_states[13], 0, 0, 0, 0 },
  };

  for (size_t i = 0; i < arraysize(hardware_state); ++i) {
    if ((i % 2) == 0) {
      base_interpreter = new PalmClassifyingFilterInterpreterTestInterpreter;
      pci.reset(new PalmClassifyingFilterInterpreter(NULL, base_interpreter,
                                                     NULL));
      wrapper.Reset(pci.get());
    }
    switch (i) {
      case 2:  // fallthough
      case 3:
      case 6:
      case 7:
        base_interpreter->expected_flags_ = 0;
        break;
      case 11:
      case 13:
        base_interpreter->expected_flags_ = GESTURES_FINGER_POSSIBLE_PALM;
        break;
      case 0:  // fallthrough
      case 1:
      case 4:
      case 5:
      case 8:
      case 9:
      case 10:
      case 12:
        base_interpreter->expected_flags_ = GESTURES_FINGER_PALM;
        break;
      default:
        ADD_FAILURE() << "Should be unreached.";
        break;
    }
    fprintf(stderr, "iteration i = %zd\n", i);
    wrapper.SyncInterpret(&hardware_state[i], NULL);
  }
}

struct PalmReevaluateTestInputs {
  stime_t now_;
  float x_, y_, pressure_;
};

// This tests that a palm that doesn't start out as a palm, but actually is,
// and can be classified as one shortly after it starts, doesn't cause motion.
TEST(PalmClassifyingFilterInterpreterTest, PalmReevaluateTest) {
  PalmClassifyingFilterInterpreter pci(NULL, NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    106.666672,  // right edge
    68.000000,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    25.4,  // screen DPI x
    25.4,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    15,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    true,  // is button pad
    0  // has_wheel
  };
  TestInterpreterWrapper wrapper(&pci, &hwprops);

  PalmReevaluateTestInputs inputs[] = {
    { 5.8174, 10.25, 46.10,  15.36 },
    { 5.8277, 10.25, 46.10,  21.18 },
    { 5.8377, 10.25, 46.10,  19.24 },
    { 5.8479,  9.91, 45.90,  15.36 },
    { 5.8578,  9.08, 44.90,  19.24 },
    { 5.8677,  9.08, 44.90,  28.94 },
    { 5.8777,  8.66, 44.70,  61.93 },
    { 5.8879,  8.41, 44.60,  58.04 },
    { 5.8973,  8.08, 44.20,  73.57 },
    { 5.9073,  7.83, 44.20,  87.15 },
    { 5.9171,  7.50, 44.40,  89.09 },
    { 5.9271,  7.25, 44.20,  87.15 },
    { 5.9369,  7.00, 44.40,  83.27 },
    { 5.9466,  6.33, 44.60,  89.09 },
    { 5.9568,  6.00, 44.70,  85.21 },
    { 5.9666,  5.75, 44.70,  87.15 },
    { 5.9763,  5.41, 44.79,  75.51 },
    { 5.9862,  5.08, 45.20,  75.51 },
    { 5.9962,  4.50, 45.50,  75.51 },
    { 6.0062,  4.41, 45.79,  81.33 },
    { 6.0160,  4.08, 46.40,  77.45 },
    { 6.0263,  3.83, 46.90,  91.03 },
    { 6.0363,  3.58, 47.50,  98.79 },
    { 6.0459,  3.25, 47.90, 114.32 },
    { 6.0560,  3.25, 47.90, 149.24 },
    { 6.0663,  3.33, 48.10, 170.59 },
    { 6.0765,  3.50, 48.29, 180.29 },
    { 6.0866,  3.66, 48.40, 188.05 },
    { 6.0967,  3.83, 48.50, 188.05 },
    { 6.1067,  3.91, 48.79, 182.23 },
    { 6.1168,  4.00, 48.79, 180.29 },
    { 6.1269,  4.08, 48.79, 180.29 },
    { 6.1370,  4.16, 48.90, 176.41 },
    { 6.1473,  4.25, 49.20, 162.82 },
    { 6.1572,  4.58, 51.00, 135.66 },
    { 6.1669,  4.66, 51.00, 114.32 },
    { 6.1767,  4.66, 51.40,  73.57 },
    { 6.1868,  4.66, 52.00,  40.58 },
    { 6.1970,  4.66, 52.40,  21.18 },
    { 6.2068,  6.25, 51.90,  13.42 },
  };
  for (size_t i = 0; i < arraysize(inputs); i++) {
    FingerState fs =
        { 0, 0, 0, 0, inputs[i].pressure_, 0.0,
          inputs[i].x_, inputs[i].y_, 1, 0 };
    HardwareState hs = { inputs[i].now_, 0, 1, 1, &fs, 0, 0, 0, 0 };

    stime_t timeout = -1.0;
    wrapper.SyncInterpret(&hs, &timeout);
    // Allow movement at first:
    stime_t age = inputs[i].now_ - inputs[0].now_;
    if (age < pci.palm_eval_timeout_.val_)
      continue;
    EXPECT_FALSE(SetContainsValue(pci.pointing_, 1));
  }
}

namespace {
struct LargeTouchMajorTestInputs {
  stime_t now_;
  float touch_major_, x_, y_, pressure_;
};
}  // namespace {}

TEST(PalmClassifyingFilterInterpreterTest, LargeTouchMajorTest) {
  PalmClassifyingFilterInterpreterTestInterpreter* base_interpreter =
      new PalmClassifyingFilterInterpreterTestInterpreter;
  PalmClassifyingFilterInterpreter pci(NULL, base_interpreter, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // x pixels/mm
    1,  // y pixels/mm
    1,  // x screen px/mm
    1,  // y screen px/mm
    -1,  // orientation minimum
    2,   // orientation maximum
    5,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0  // has_wheel
  };
  TestInterpreterWrapper wrapper(&pci, &hwprops);

  LargeTouchMajorTestInputs inputs[] = {
    { 658.0355, 22.73, 29.1, 29.1, 140.6 },
    { 658.0475, 22.73, 29.1, 29.1, 140.6 },
    { 658.0652, 23.51, 29.1, 29.1, 118.1 },
    { 658.0827, 22.73, 29.1, 29.1, 161.7 },
    { 658.1002, 20.20, 29.1, 29.1, 156.4 },
    { 658.1171, 21.95, 29.2, 29.5, 144.5 },
    { 658.1344, 25.06, 29.2, 29.9, 157.8 },
    { 658.1513, 26.42, 28.9, 30.0, 144.5 },
    { 658.1687, 22.73, 28.9, 29.8, 120.7 },
    { 658.1856, 25.06, 28.9, 30.1, 159.1 },
    { 658.2033, 23.51, 28.9, 30.3, 174.9 },
    { 658.2204, 22.73, 28.9, 30.1, 161.7 },
    { 658.2378, 17.28, 29.6, 30.1, 144.5 },
    { 658.2561, 21.95, 29.9, 32.0, 144.5 },
    { 658.2737, 20.20, 30.2, 30.7, 144.5 },
    { 658.2907, 16.21, 31.4, 30.7, 145.9 },
    { 658.3079, 22.73, 30.7, 32.6, 148.5 },
    { 658.3259, 21.07, 30.8, 30.5, 124.7 },
    { 658.3437, 22.73, 30.7, 30.4, 147.2 },
    { 658.3613, 21.95, 30.6, 32.7, 141.9 },
    { 658.3782, 25.74, 30.2, 32.6, 148.5 },
    { 658.3954, 25.74, 29.3, 31.9, 151.2 },
    { 658.4135, 25.06, 28.2, 34.9, 147.2 },
    { 658.4307, 25.06, 28.9, 29.1, 141.9 },
    { 658.4476, 25.74, 29.0, 31.7, 145.9 },
    { 658.4642, 25.74, 29.0, 32.0, 151.2 },
    { 658.4811, 29.15, 27.6, 32.7, 141.9 },
    { 658.4984, 27.11, 27.5, 32.2, 148.5 },
    { 658.5150, 25.06, 27.9, 32.3, 148.5 },
    { 658.5322, 27.11, 27.8, 32.2, 147.2 },
    { 658.5494, 27.79, 27.8, 32.1, 151.2 },
    { 658.5667, 20.20, 28.5, 31.2, 123.4 },
    { 658.5846, 25.06, 28.6, 31.4, 161.7 },
    { 658.6022, 21.07, 29.3, 30.5, 126.0 },
    { 658.6195, 25.74, 29.2, 30.9, 152.5 },
    { 658.6372, 29.15, 27.8, 32.0, 151.2 },
    { 658.6547, 29.73, 27.1, 32.6, 145.9 },
    { 658.6724, 25.06, 27.5, 32.7, 176.3 },
    { 658.6910, 27.79, 27.4, 32.4, 141.9 },
    { 658.7081, 27.11, 27.5, 32.2, 141.9 },
    { 658.7263, 27.79, 27.4, 32.2, 147.2 },
    { 658.7428, 27.11, 27.3, 32.2, 145.9 },
    { 658.7601, 25.74, 27.5, 32.2, 148.5 },
    { 658.7780, 16.21, 29.0, 31.8, 147.2 },
    { 658.7956, 25.74, 29.0, 31.9, 152.5 },
    { 658.8132, 21.07, 29.4, 31.1, 145.9 },
    { 658.8311, 27.79, 29.0, 31.1, 145.9 },
    { 658.8493, 25.74, 28.9, 31.3, 147.2 },
    { 658.8666, 24.28, 29.0, 31.3, 147.2 },
    { 658.8840, 25.06, 29.0, 31.1, 147.2 },
    { 658.9014, 29.15, 28.7, 31.0, 152.5 },
    { 658.9138, 29.15, 28.7, 31.0, 152.5 },
    { 658.9309, 27.79, 28.6, 31.0, 152.5 },
    { 658.9491, 25.06, 28.6, 31.1, 148.5 },
    { 658.9667, 27.79, 28.5, 31.1, 152.5 },
    { 658.9847, 25.74, 28.5, 31.2, 143.2 },
    { 659.0022, 25.74, 28.6, 31.3, 149.8 },
    { 659.0193, 20.20, 28.8, 30.9, 122.1 },
    { 659.0371, 29.73, 28.1, 31.6, 151.2 },
    { 659.0550, 21.07, 29.2, 30.2, 145.9 },
    { 659.0728, 16.21, 31.3, 30.5, 145.9 },
    { 659.0911, 27.79, 27.2, 31.8, 127.4 },
    { 659.1092, 26.42, 27.6, 31.4, 180.2 },
    { 659.1275, 23.51, 29.8, 34.1, 148.5 },
    { 659.1444, 25.74, 29.3, 33.4, 141.9 },
    { 659.1631, 25.74, 29.2, 33.1, 152.5 },
    { 659.1808, 25.74, 29.2, 33.0, 149.8 },
    { 659.1986, 21.95, 28.1, 29.9, 165.7 },
    { 659.2168, 27.11, 27.8, 30.4, 148.5 },
    { 659.2346, 27.11, 27.8, 30.6, 149.8 },
    { 659.2521, 29.73, 27.2, 31.5, 153.8 },
    { 659.2699, 25.06, 27.6, 31.8, 152.5 },
    { 659.2869, 25.74, 28.0, 32.0, 152.5 },
    { 659.3041, 23.51, 28.4, 32.5, 143.2 },
    { 659.3223, 25.74, 28.5, 32.5, 149.8 },
    { 659.3402, 23.51, 28.5, 31.6, 147.2 },
    { 659.3583, 24.28, 28.7, 31.6, 152.5 },
    { 659.3756, 27.79, 28.6, 31.6, 149.8 },
    { 659.3933, 29.15, 28.3, 31.7, 164.4 },
    { 659.4100, 25.74, 28.4, 31.8, 152.5 },
    { 659.4276, 29.73, 27.9, 32.2, 145.9 },
    { 659.4449, 23.51, 28.8, 33.1, 148.5 },
    { 659.4617, 25.74, 28.8, 33.0, 151.2 },
    { 659.4785, 25.74, 28.9, 33.0, 152.5 },
    { 659.4963, 29.15, 28.6, 33.0, 181.6 },
    { 659.5135, 20.20, 29.7, 32.6, 149.8 },
    { 659.5303, 29.15, 28.9, 32.6, 152.5 },
    { 659.5482, 26.42, 28.5, 34.1, 126.0 },
    { 659.5661, 25.74, 28.7, 33.7, 127.4 },
    { 659.5839, 22.73, 29.1, 33.6, 151.2 },
    { 659.6017, 25.74, 29.1, 33.5, 149.8 },
    { 659.6198, 27.79, 28.8, 33.1, 143.2 },
    { 659.6382, 26.42, 28.6, 32.6, 182.9 },
    { 659.6548, 25.06, 28.7, 32.6, 163.0 },
    { 659.6726, 25.74, 28.7, 32.6, 149.8 },
    { 659.6898, 25.06, 28.7, 32.6, 143.2 },
    { 659.7072, 25.74, 28.7, 32.6, 152.5 },
    { 659.7248, 27.79, 28.7, 32.5, 156.4 },
    { 659.7421, 25.74, 28.7, 32.5, 178.9 },
    { 659.7586, 25.74, 28.7, 32.5, 153.8 },
    { 659.7774, 27.79, 28.6, 32.5, 149.8 },
    { 659.7950, 25.74, 28.7, 32.5, 143.2 },
    { 659.8126, 25.74, 28.7, 32.5, 151.2 },
    { 659.8304, 24.28, 28.8, 32.6, 153.8 },
    { 659.8477, 29.15, 28.6, 32.6, 128.7 },
    { 659.8655, 27.79, 28.5, 32.9, 130.0 },
    { 659.8827, 25.74, 28.5, 32.9, 165.7 },
    { 659.9001, 27.79, 28.4, 32.7, 149.8 },
    { 659.9179, 27.79, 28.3, 32.5, 148.5 },
    { 659.9351, 25.06, 28.5, 31.9, 149.8 },
    { 659.9530, 25.74, 28.6, 31.9, 149.8 },
    { 659.9701, 29.73, 28.1, 32.2, 151.2 },
    { 659.9874, 27.79, 28.0, 32.1, 143.2 },
    { 660.0051, 25.74, 28.1, 32.2, 141.9 },
    { 660.0227, 25.74, 28.2, 32.2, 152.5 },
    { 660.0403, 23.51, 28.7, 32.7, 151.2 },
    { 660.0576, 27.79, 28.5, 32.5, 147.2 },
    { 660.0755, 25.74, 28.5, 32.5, 178.9 },
    { 660.0930, 22.73, 29.1, 31.9, 127.4 },
    { 660.1101, 22.73, 29.5, 31.4, 149.8 },
    { 660.1273, 27.79, 29.2, 31.3, 128.7 },
    { 660.1449, 24.28, 29.2, 31.4, 151.2 },
    { 660.1632, 28.47, 29.0, 31.5, 151.2 },
    { 660.1803, 25.06, 29.0, 31.2, 149.8 },
    { 660.1992, 23.51, 29.4, 32.2, 147.2 },
    { 660.2168, 22.73, 29.6, 32.9, 149.8 },
    { 660.2350, 22.73, 30.1, 31.8, 151.2 },
    { 660.2524, 25.74, 29.9, 32.0, 145.9 },
    { 660.2708, 29.73, 28.0, 32.7, 174.9 },
    { 660.2883, 27.79, 27.9, 32.3, 151.2 },
    { 660.3063, 25.74, 28.1, 32.3, 151.2 },
    { 660.3232, 25.74, 28.2, 32.4, 153.8 },
    { 660.3401, 25.74, 28.3, 32.4, 160.4 },
    { 660.3576, 24.28, 28.5, 32.6, 153.8 },
    { 660.3748, 27.79, 28.4, 32.5, 151.2 },
    { 660.3920, 25.74, 28.5, 32.5, 155.1 },
    { 660.4099, 23.51, 28.5, 31.8, 148.5 },
    { 660.4280, 25.74, 28.6, 31.9, 151.2 },
    { 660.4448, 25.74, 28.6, 32.0, 127.4 },
    { 660.4612, 25.74, 28.7, 32.0, 151.2 },
    { 660.4799, 25.06, 28.7, 32.1, 164.4 },
    { 660.4971, 25.74, 28.7, 32.1, 148.5 },
    { 660.5155, 29.15, 28.5, 32.1, 168.3 },
    { 660.5334, 27.79, 28.5, 32.1, 151.2 },
    { 660.5507, 25.74, 28.5, 32.1, 156.4 },
    { 660.5690, 25.06, 28.5, 32.1, 145.9 },
    { 660.5868, 23.51, 28.8, 32.5, 140.6 },
    { 660.6049, 25.74, 28.8, 32.5, 155.1 },
    { 660.6217, 25.74, 28.8, 32.6, 152.5 },
    { 660.6394, 27.79, 28.8, 32.5, 141.9 },
    { 660.6571, 23.51, 29.0, 32.8, 145.9 },
    { 660.6749, 29.73, 28.4, 33.0, 145.9 },
    { 660.6924, 25.06, 28.5, 32.9, 151.2 },
    { 660.7088, 25.74, 28.6, 32.9, 153.8 },
    { 660.7268, 24.28, 28.7, 32.7, 152.5 },
    { 660.7443, 25.74, 28.7, 32.7, 152.5 },
    { 660.7617, 25.74, 28.7, 32.7, 157.8 },
    { 660.7790, 23.51, 28.9, 33.0, 152.5 },
    { 660.7969, 25.74, 28.9, 33.0, 135.3 },
    { 660.8136, 25.74, 29.0, 32.9, 128.7 },
    { 660.8308, 25.74, 29.0, 32.9, 136.6 },
    { 660.8489, 20.20, 29.4, 32.7, 151.2 },
    { 660.8664, 25.06, 28.6, 31.3, 152.5 },
    { 660.8837, 23.51, 29.2, 32.4, 153.8 },
    { 660.9014, 25.74, 29.2, 32.5, 152.5 },
    { 660.9190, 27.79, 28.9, 32.4, 148.5 },
    { 660.9358, 24.28, 29.2, 32.7, 155.1 },
    { 660.9523, 25.74, 29.2, 32.6, 149.8 },
    { 660.9698, 25.74, 29.2, 32.6, 155.1 },
    { 660.9872, 24.28, 29.3, 32.7, 152.5 },
    { 661.0051, 25.74, 29.3, 32.7, 132.6 },
    { 661.0233, 29.15, 28.9, 32.5, 147.2 },
    { 661.0404, 22.73, 29.3, 32.0, 153.8 },
    { 661.0584, 29.73, 28.3, 32.5, 152.5 },
    { 661.0758, 25.74, 28.4, 32.5, 156.4 },
    { 661.0940, 18.35, 30.9, 31.1, 155.1 },
    { 661.1123, 18.35, 31.9, 30.5, 155.1 },
    { 661.1302, 23.51, 29.9, 32.9, 193.5 },
    { 661.1471, 24.28, 30.0, 33.4, 127.4 },
    { 661.1657, 25.06, 29.6, 33.6, 151.2 },
    { 661.1836, 26.42, 29.0, 32.4, 155.1 },
    { 661.2009, 25.74, 29.1, 32.5, 156.4 },
    { 661.2189, 25.74, 28.9, 32.1, 178.9 },
    { 661.2368, 29.15, 28.4, 32.2, 151.2 },
    { 661.2543, 23.51, 29.2, 33.1, 127.4 },
    { 661.2720, 25.74, 29.1, 33.1, 151.2 },
    { 661.2894, 27.79, 28.9, 32.7, 132.6 },
    { 661.3069, 22.73, 29.4, 32.0, 152.5 },
    { 661.3249, 23.51, 30.0, 33.2, 151.2 },
    { 661.3432, 25.74, 29.3, 35.1, 153.8 },
    { 661.3600, 25.74, 29.3, 34.2, 155.1 },
    { 661.3775, 27.79, 28.6, 32.9, 156.4 },
    { 661.3947, 25.74, 28.7, 32.9, 181.6 },
    { 661.4111, 24.28, 29.0, 33.1, 130.0 },
    { 661.4299, 27.79, 28.7, 32.7, 151.2 },
    { 661.4471, 25.74, 28.7, 32.7, 153.8 },
    { 661.4648, 25.74, 28.8, 32.6, 153.8 },
    { 661.4820, 27.79, 28.7, 32.5, 147.2 },
    { 661.4999, 27.79, 28.6, 32.4, 152.5 },
    { 661.5167, 25.74, 28.6, 32.4, 167.0 },
    { 661.5334, 25.74, 28.7, 32.4, 152.5 },
    { 661.5498, 24.28, 28.8, 32.5, 151.2 },
    { 661.5680, 25.06, 28.9, 32.1, 136.6 },
    { 661.5863, 25.06, 29.0, 31.7, 151.2 },
    { 661.6037, 25.74, 29.0, 31.8, 132.6 },
    { 661.6218, 25.74, 29.0, 31.8, 155.1 },
    { 661.6386, 25.74, 29.1, 31.9, 144.5 },
    { 661.6562, 20.20, 29.5, 31.8, 151.2 },
    { 661.6739, 25.74, 29.4, 31.9, 155.1 },
    { 661.6907, 22.73, 29.5, 32.2, 155.1 },
    { 661.7087, 20.20, 30.2, 32.1, 155.1 },
    { 661.7256, 24.28, 30.2, 32.4, 152.5 },
    { 661.7429, 24.28, 30.1, 32.3, 153.8 },
    { 661.7605, 25.74, 30.1, 32.4, 156.4 },
    { 661.7775, 25.74, 30.0, 32.4, 159.1 },
    { 661.7956, 25.74, 30.0, 32.4, 151.2 },
    { 661.8128, 24.28, 30.0, 32.5, 152.5 },
    { 661.8301, 24.28, 29.3, 31.6, 151.2 },
    { 661.8480, 22.73, 29.3, 31.6, 161.7 },
    { 661.8634, 16.21, 29.3, 30.8, 151.2 },
    { 661.8793, 18.35, 28.3, 29.6, 141.9 },
    { 661.8893, 18.35, 28.3, 29.6, 141.9 },
    { 661.8969, 18.35, 28.3, 29.6, 141.9 },
    { 661.9044, 18.35, 28.3, 29.6, 141.9 },
    { 661.9111, 18.35, 28.3, 29.6, 141.9 }
  };

  for (size_t i = 0; i < arraysize(inputs); i++) {
    const LargeTouchMajorTestInputs& input = inputs[i];
    FingerState fs = {
      input.touch_major_, 0, 0, 0, input.pressure_, 0, input.x_, input.y_, 1, 0
    };
    HardwareState hs = { input.now_, 0, 1, 1, &fs, 0, 0, 0, 0 };
    base_interpreter->expected_flags_ = GESTURES_FINGER_PALM;
    wrapper.SyncInterpret(&hs, NULL);
  }
}

}  // namespace gestures
