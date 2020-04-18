// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include <deque>
#include <vector>

#include "gestures/include/gestures.h"
#include "gestures/include/cr48_profile_sensor_filter_interpreter.h"
#include "gestures/include/unittest_util.h"

using std::deque;
using std::make_pair;
using std::pair;

namespace gestures {

class Cr48ProfileSensorFilterInterpreterTest : public ::testing::Test {};

class Cr48ProfileSensorFilterInterpreterTestInterpreter : public Interpreter {
 public:
  Cr48ProfileSensorFilterInterpreterTestInterpreter()
        : Interpreter(NULL, NULL, false), sync_interpret_cnt_(0) {
  }

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    sync_interpret_cnt_++;

    if (!expected_coordinates_.empty()) {
      std::vector<FingerPosition>& expected = expected_coordinates_.front();
      for (size_t i = 0; i < hwstate->finger_cnt; i++) {
        EXPECT_FLOAT_EQ(expected[i].x, hwstate->fingers[i].position_x)
            << "i = " << i;
        EXPECT_FLOAT_EQ(expected[i].y, hwstate->fingers[i].position_y)
            << "i = " << i;
      }
      expected_coordinates_.pop_front();
    }
    if (!expected_finger_cnt_.empty()) {
      EXPECT_EQ(expected_finger_cnt_.front(), hwstate->finger_cnt);
      expected_finger_cnt_.pop_front();
    }

    if (!expected_touch_cnt_.empty()) {
      EXPECT_EQ(expected_touch_cnt_.front(), hwstate->touch_cnt);
      expected_touch_cnt_.pop_front();
    }

    if (!unexpected_tracking_id_.empty() && (hwstate->finger_cnt > 0)) {
      EXPECT_NE(unexpected_tracking_id_.front(),
                hwstate->fingers[0].tracking_id);
      unexpected_tracking_id_.pop_front();
    }

    if (!expected_tracking_id_.empty()) {
      std::vector<short>& expected = expected_tracking_id_.front();
      for (size_t i = 0; i < hwstate->finger_cnt; i++) {
        EXPECT_EQ(expected[i], hwstate->fingers[i].tracking_id)
            << "i = " << i;
      }
      expected_tracking_id_.pop_front();
    }
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {}

  int sync_interpret_cnt_;
  deque<std::vector<FingerPosition> > expected_coordinates_;
  deque<float> expected_pressures_;
  deque<int> expected_finger_cnt_;
  deque<int> expected_touch_cnt_;
  deque<int> unexpected_tracking_id_;
  deque<std::vector<short> > expected_tracking_id_;
};

TEST(Cr48ProfileSensorFilterInterpreterTest, LowPressureTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrIDm, flags
    { 0, 0, 0, 0, 40, 0, 1, 1, 5, 0 },
    { 0, 0, 0, 0, 28, 0, 2, 2, 5, 0 },
    { 0, 0, 0, 0, 20, 0, 2, 2, 5, 0 },
    { 0, 0, 0, 0, 40, 0, 3, 3, 5, 0 },
    { 0, 0, 0, 0, 20, 0, 1, 1, 5, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  };
  HardwareState hs[] = {
    { 0.000, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 0.010, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    { 0.020, 0, 1, 1, &fs[2], 0, 0, 0, 0 },
    { 0.030, 0, 1, 1, &fs[3], 0, 0, 0, 0 },
    { 0.040, 0, 1, 1, &fs[4], 0, 0, 0, 0 },
  };

  HardwareProperties hwprops = {
    0, 0, 100, 60,  // left, top, right, bottom
    1.0, 1.0, 25.4, 25.4,  // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3, 0, 0, 0, 0  // max_fingers, max_touch, t5r2, semi_mt,
  };

  hwprops.support_semi_mt = true;
  TestInterpreterWrapper wrapper(&interpreter, &hwprops);
  interpreter.interpreter_enabled_.val_ = true;

  base_interpreter->expected_finger_cnt_.push_back(1);
  wrapper.SyncInterpret(&hs[0], NULL);
  int current_tracking_id = fs[0].tracking_id;

  base_interpreter->expected_finger_cnt_.push_back(1);
  wrapper.SyncInterpret(&hs[1], NULL);

  base_interpreter->expected_finger_cnt_.push_back(0);
  wrapper.SyncInterpret(&hs[2], NULL);

  base_interpreter->expected_finger_cnt_.push_back(1);
  base_interpreter->unexpected_tracking_id_.push_back(current_tracking_id);
  wrapper.SyncInterpret(&hs[3], NULL);

  base_interpreter->expected_finger_cnt_.push_back(0);
  wrapper.SyncInterpret(&hs[4], NULL);
}

TEST(Cr48ProfileSensorFilterInterpreterTest, TrackingIdMappingTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 40, 0, 3, 3, 10, 0 },
    { 0, 0, 0, 0, 40, 0, 3, 3, 10, 0 },

    { 0, 0, 0, 0, 40, 0, 3, 3, 16, 0 },
    { 0, 0, 0, 0, 40, 0, 5, 5, 17, 0 },

    { 0, 0, 0, 0, 40, 0, 3, 3, 16, 0 },
    { 0, 0, 0, 0, 40, 0, 5, 5, 17, 0 },
  };
  HardwareState hs[] = {
    { 0.000, 0, 0, 0, &fs[0], 0, 0, 0, 0 },
    { 0.010, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 0.020, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    { 0.030, 0, 0, 0, &fs[1], 0, 0, 0, 0 },
    { 0.040, 0, 2, 2, &fs[2], 0, 0, 0, 0 },
    { 0.050, 0, 2, 2, &fs[4], 0, 0, 0, 0 },
  };

  HardwareProperties hwprops = {
    0, 0, 100, 60,  // left, top, right, bottom
    1.0, 1.0, 25.4, 25.4,  // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3, 0, 0, 0, 0  // max_fingers, max_touch, t5r2, semi_mt,
  };

  hwprops.support_semi_mt = true;
  TestInterpreterWrapper wrapper(&interpreter, &hwprops);
  interpreter.interpreter_enabled_.val_ = true;

  wrapper.SyncInterpret(&hs[0], NULL);
  wrapper.SyncInterpret(&hs[1], NULL);

  std::vector<short>  result;
  result.push_back(hs[1].fingers[0].tracking_id);
  base_interpreter->expected_tracking_id_.push_back(result);
  short original_id = hs[2].fingers[0].tracking_id;
  base_interpreter->unexpected_tracking_id_.push_back(original_id);
  wrapper.SyncInterpret(&hs[2], NULL);

  wrapper.SyncInterpret(&hs[3], NULL);
  wrapper.SyncInterpret(&hs[4], NULL);
  result.clear();
  result.push_back(hs[4].fingers[0].tracking_id);
  result.push_back(hs[4].fingers[1].tracking_id);
  base_interpreter->expected_tracking_id_.push_back(result);
  wrapper.SyncInterpret(&hs[5], NULL);
}

TEST(Cr48ProfileSensorFilterInterpreterTest, CorrectFingerPositionTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    { 0, 0, 0, 0, 60, 0, 4000, 3300, 5, 0 },
    { 0, 0, 0, 0, 60, 0, 2900, 3300, 5, 0 },
    { 0, 0, 0, 0, 60, 0, 4000, 2700, 6, 0 },
    { 0, 0, 0, 0, 60, 0, 2950, 3200, 5, 0 },
    { 0, 0, 0, 0, 60, 0, 4050, 2750, 6, 0 },
  };
  HardwareState hs[] = {
    { 0.000, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 0.010, 0, 2, 2, &fs[1], 0, 0, 0, 0 },
    { 0.010, 0, 2, 2, &fs[3], 0, 0, 0, 0 },
  };

  HardwareProperties hwprops = {
    1400, 1400, 5600, 4500,  // left, top, right, bottom
    1.0, 1.0, 25.4, 25.4,  // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3, 0, 0, 0, 0  // max_fingers, max_touch, t5r2, semi_mt,
  };

  hwprops.support_semi_mt = true;
  TestInterpreterWrapper wrapper(&interpreter, &hwprops);
  interpreter.interpreter_enabled_.val_ = true;
  interpreter.bounding_box_.val_ = 1;

  wrapper.SyncInterpret(&hs[0], NULL);

  // Test if both finger positions are corrected.
  std::vector<FingerPosition>  result;
  FingerPosition first_finger_position1 = { 4000, 3300 };
  result.push_back(first_finger_position1);  // first finger
  FingerPosition second_finger_position1 = { 2900, 2700 };
  result.push_back(second_finger_position1);  // second finger
  base_interpreter->expected_coordinates_.push_back(result);
  wrapper.SyncInterpret(&hs[1], NULL);

  result.clear();
  FingerPosition first_finger_position2 = { 4050, 3200 };
  result.push_back(first_finger_position2);  // first finger
  FingerPosition second_finger_position2 = { 2950, 2750 };
  result.push_back(second_finger_position2);  // second finger
  base_interpreter->expected_coordinates_.push_back(result);
  wrapper.SyncInterpret(&hs[2], NULL);
}

TEST(Cr48ProfileSensorFilterInterpreterTest, FingerCrossOverTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 60, 0, 2969, 3088, 1481, 0},

    // Test if we reflect the pattern change for one-finger vertical
    // scroll, the starting finger pattern is left-bottom-right-top.
    // index 1
    { 0, 0, 0, 0, 60, 0, 2969, 3088, 1481, 0},
    { 0, 0, 0, 0, 60, 0, 4121, 1938, 1482, 0},

    // index 3
    { 0, 0, 0, 0, 60, 0, 2969, 2978, 1481, 0},
    { 0, 0, 0, 0, 60, 0, 4121, 2421, 1482, 0},

    // index 5
    { 0, 0, 0, 0, 60, 0, 2969, 3038, 1481, 0},
    { 0, 0, 0, 0, 60, 0, 4121, 3001, 1482, 0},

    // index 7
    { 0, 0, 0, 0, 60, 0, 2969, 3056, 1481, 0},
    { 0, 0, 0, 0, 60, 0, 4120, 3043, 1482, 0},

    // index 9
    { 0, 0, 0, 0, 60, 0, 2969, 3082, 1481, 0},
    { 0, 0, 0, 0, 60, 0, 4118, 3076, 1482, 0},

    // finger with tid 1481 crosses the finger 1482 vertically
    // we should see the position_y swapped in the results, i.e.
    // finger pattern is left-top-right-bottom.
    // index 11
    { 0, 0, 0, 0, 60, 0, 2969, 3117, 1481, 0},
    { 0, 0, 0, 0, 60, 0, 4118, 3096, 1482, 0},

    // index 13
    { 0, 0, 0, 0, 60, 0, 2969, 3153, 1481, 0},
    { 0, 0, 0, 0, 60, 0, 4118, 3114, 1482, 0},

    // index 15
    { 0, 0, 0, 0, 60, 0, 2969, 3198, 1481, 0},
    { 0, 0, 0, 0, 60, 0, 4118, 3130, 1482, 0},
  };
  HardwareState hs[] = {
    { 0.000, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 0.000, 0, 2, 2, &fs[1], 0, 0, 0, 0 },
    { 0.010, 0, 2, 2, &fs[3], 0, 0, 0, 0 },
    { 0.020, 0, 2, 2, &fs[5], 0, 0, 0, 0 },
    { 0.030, 0, 2, 2, &fs[7], 0, 0, 0, 0 },
    { 0.040, 0, 2, 2, &fs[9], 0, 0, 0, 0 },
    { 0.050, 0, 2, 2, &fs[11], 0, 0, 0, 0 },
    // the finger pattern will be swapped
    { 0.060, 0, 2, 2, &fs[13], 0, 0, 0, 0 },
    // to left-top-right-bottom and should
    { 0.060, 0, 2, 2, &fs[15], 0, 0, 0, 0 },
    // applied for the following reports
  };

  HardwareProperties hwprops = {
    1400, 1400, 5600, 4500,  // left, top, right, bottom
    1.0, 1.0, 25.4, 25.4,  // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3, 0, 0, 0, 0  // max_fingers, max_touch, t5r2, semi_mt,
  };

  size_t hwstate_index_finger_crossed = 6;

  hwprops.support_semi_mt = true;
  TestInterpreterWrapper wrapper(&interpreter, &hwprops);
  interpreter.interpreter_enabled_.val_ = true;
  interpreter.bounding_box_.val_ = 1;

  for (size_t i = 0; i < hwstate_index_finger_crossed; i++)
    wrapper.SyncInterpret(&hs[i], NULL);

  // Test if both finger positions are corrected with the new pattern
  // by examining the swapped position_y values.
  std::vector<FingerPosition>  result;
  FingerPosition first_finger_position1 = { 2969, 3096 };
  result.push_back(first_finger_position1);  // first finger
  FingerPosition second_finger_position1 = { 4118, 3117 };
  result.push_back(second_finger_position1);  // second finger
  base_interpreter->expected_coordinates_.push_back(result);
  wrapper.SyncInterpret(&hs[hwstate_index_finger_crossed], NULL);

  result.clear();
  FingerPosition first_finger_position2 = { 2969, 3114 };
  result.push_back(first_finger_position2);  // first finger
  FingerPosition second_finger_position2 = { 4118, 3153 };
  result.push_back(second_finger_position2);  // second finger
  base_interpreter->expected_coordinates_.push_back(result);
  wrapper.SyncInterpret(&hs[hwstate_index_finger_crossed + 1], NULL);

  result.clear();
  FingerPosition first_finger_position3 = { 2969, 3130 };
  result.push_back(first_finger_position3);  // first finger
  FingerPosition second_finger_position3 =  { 4118, 3198 };
  result.push_back(second_finger_position3);  // second finger
  base_interpreter->expected_coordinates_.push_back(result);
  wrapper.SyncInterpret(&hs[hwstate_index_finger_crossed + 2], NULL);
}

TEST(Cr48ProfileSensorFilterInterpreterTest, ClipNonLinearAreaTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    { 0, 0, 0, 0, 60, 0, 1240, 3088, 1481, 0},
    { 0, 0, 0, 0, 60, 0, 5570, 3088, 1481, 0},
    { 0, 0, 0, 0, 60, 0, 4118, 1210, 1481, 0},
    { 0, 0, 0, 0, 60, 0, 4118, 4580, 1481, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  };
  HardwareState hs[] = {
    { 0.00, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 0.02, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    { 0.04, 0, 1, 1, &fs[2], 0, 0, 0, 0 },
    { 0.06, 0, 1, 1, &fs[3], 0, 0, 0, 0 },
  };

  interpreter.non_linear_left_.val_ = 1360.0;
  interpreter.non_linear_right_.val_ = 5560.0;
  interpreter.non_linear_top_.val_ = 1250.0;
  interpreter.non_linear_bottom_.val_ = 4570.0;

  // Test if finger positions are corrected when a finger is located
  // in the non-linear clipping area.
  interpreter.ClipNonLinearFingerPosition(&hs[0]);
  EXPECT_EQ(interpreter.non_linear_left_.val_, fs[0].position_x);

  interpreter.ClipNonLinearFingerPosition(&hs[1]);
  EXPECT_EQ(interpreter.non_linear_right_.val_, fs[1].position_x);

  interpreter.ClipNonLinearFingerPosition(&hs[2]);
  EXPECT_EQ(interpreter.non_linear_top_.val_, fs[2].position_y);

  interpreter.ClipNonLinearFingerPosition(&hs[3]);
  EXPECT_EQ(interpreter.non_linear_bottom_.val_, fs[3].position_y);
}

TEST(Cr48ProfileSensorFilterInterpreterTest, MovingFingerTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  // Test one, second finger arrives below the first finger, then move the first
  // finger. Expect moving finger will be the first finger(Y=1942) as the
  // second finger's starting Y(Y=4370) is higher.
  FingerState fs[] = {

    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 80, 0, 3424, 1942, 48, 0},

    // index 1
    { 0, 0, 0, 0, 80, 0, 3426, 4370, 48, 0},
    { 0, 0, 0, 0, 80, 0, 3598, 1938, 49, 0},

    // index 3
    { 0, 0, 0, 0, 80, 0, 3427, 4406, 48, 0},
    { 0, 0, 0, 0, 80, 0, 3502, 1934, 49, 0},

    // index 5
    { 0, 0, 0, 0, 80, 0, 3458, 4449, 48, 0},
    { 0, 0, 0, 0, 80, 0, 3516, 1920, 49, 0},

    // index 7
    { 0, 0, 0, 0, 80, 0, 3480, 4449, 48, 0},
    { 0, 0, 0, 0, 80, 0, 3521, 1920, 49, 0},

    // index 9
    { 0, 0, 0, 0, 80, 0, 3486, 4449, 48, 0},
    { 0, 0, 0, 0, 80, 0, 3527, 1918, 49, 0},

    // index 11
    { 0, 0, 0, 0, 80, 0, 3492, 4449, 48, 0},
    { 0, 0, 0, 0, 80, 0, 3532, 1918, 49, 0},

    // index 13
    { 0, 0, 0, 0, 80, 0, 3510, 4449, 48, 0},
    { 0, 0, 0, 0, 80, 0, 3539, 1918, 49, 0},

    // index 15
    { 0, 0, 0, 0, 80, 0, 3516, 4449, 48, 0},
    { 0, 0, 0, 0, 80, 0, 3546, 1918, 49, 0},

    // index 17
    { 0, 0, 0, 0, 80, 0, 3539, 4449, 48, 0},
    { 0, 0, 0, 0, 80, 0, 3564, 1918, 49, 0},

    // index 19
    { 0, 0, 0, 0, 80, 0, 3546, 4449, 48, 0},
    { 0, 0, 0, 0, 80, 0, 3568, 1918, 49, 0},
  };

  // Test two, have a resting thumb in the center of the touchpad, then the
  // index finger tries to cross the thumb horizontally. Expect the moving
  // finger will be the index finger as its starting position (Y=1873) is lower
  // than the first finger (Y=3325).
  FingerState fs2[] = {
    { 0, 0, 0, 0, 108, 0, 3202, 3325, 99, 0},

    // index 1
    { 0, 0, 0, 0, 108, 0, 3198, 3325, 99, 0},
    { 0, 0, 0, 0, 108, 0, 2424, 1873, 100, 0},

    // index 3
    { 0, 0, 0, 0, 108, 0, 3198, 3325, 99, 0},
    { 0, 0, 0, 0, 108, 0, 2471, 1873, 100, 0},

    // index 5
    { 0, 0, 0, 0, 108, 0, 3196, 3325, 99, 0},
    { 0, 0, 0, 0, 108, 0, 2500, 1867, 100, 0},

    // index 7
    { 0, 0, 0, 0, 108, 0, 3192, 3325, 99, 0},
    { 0, 0, 0, 0, 108, 0, 2546, 1850, 100, 0},

    // index 9
    { 0, 0, 0, 0, 108, 0, 3185, 3325, 99, 0},
    { 0, 0, 0, 0, 108, 0, 2594, 1844, 100, 0},

    // index 11
    { 0, 0, 0, 0, 108, 0, 3177, 3325, 99, 0},
    { 0, 0, 0, 0, 108, 0, 2654, 1822, 100, 0},

    // index 13
    { 0, 0, 0, 0, 108, 0, 3152, 3325, 99, 0},
    { 0, 0, 0, 0, 108, 0, 2707, 1820, 100, 0},

    // index 15
    { 0, 0, 0, 0, 108, 0, 3130, 3325, 99, 0},
    { 0, 0, 0, 0, 108, 0, 2749, 1818, 100, 0},

    // index 17
    { 0, 0, 0, 0, 108, 0, 3098, 3325, 99, 0},
    { 0, 0, 0, 0, 108, 0, 2780, 1801, 100, 0},

    // index 19
    { 0, 0, 0, 0, 108, 0, 3082, 3325, 99, 0},
    { 0, 0, 0, 0, 108, 0, 2804, 1797, 100, 0},
  };

  HardwareState hs[] = {
    { 0.00, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 0.02, 0, 2, 2, &fs[1], 0, 0, 0, 0 },
  };

  HardwareState hs2[] = {
    { 0.00, 0, 1, 1, &fs2[0], 0, 0, 0, 0 },
    { 0.02, 0, 2, 2, &fs2[1], 0, 0, 0, 0 },
  };

  HardwareProperties hwprops = {
    1217, 5733, 1061, 4798,  // left, top, right, bottom
    1.0, 1.0, 133, 133,  // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3, 0, 1, 1, 0  // max_fingers, max_touch, t5r2, semi_mt, is_button_pad
  };

  TestInterpreterWrapper wrapper(&interpreter, &hwprops);
  interpreter.interpreter_enabled_.val_ = true;
  interpreter.bounding_box_.val_ = 1;

  // Test one, the moving finger should be the first finger.
  wrapper.SyncInterpret(&hs[0], NULL);
  for (size_t i = 0; i < (arraysize(fs) - 1) / 2; i++) {
    hs[1].fingers = &fs[2 * i + 1];  // update the finger array
    hs[1].timestamp += 0.02;
    wrapper.SyncInterpret(&hs[1], NULL);
    EXPECT_EQ(interpreter.moving_finger_, 0);
  }

  // Test two, the moving finger should be the second finger.
  wrapper.SyncInterpret(&hs2[0], NULL);
  for (size_t i = 0; i < (arraysize(fs2) - 1) / 2; i++) {
    hs2[1].fingers = &fs2[2 * i + 1];  // update the finger array
    hs2[1].timestamp += 0.02;
    wrapper.SyncInterpret(&hs2[1], NULL);
    EXPECT_EQ(interpreter.moving_finger_, 1);
  }
}

const struct FingerState* kNullFingers = NULL;

TEST(Cr48ProfileSensorFilterInterpreterTest, HistoryTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 100, 0, 3000, 1800, 67, 0},
    { 0, 0, 0, 0, 110, 0, 3100, 2000, 68, 0},
    { 0, 0, 0, 0, 120, 0, 3200, 2200, 69, 0},
  };

  HardwareState hs[] = {
    // time, buttons, finger count, touch count, fingers
    { 0.500, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 0.525, 0, 2, 2, &fs[1], 0, 0, 0, 0 },
  };

  HardwareProperties hwprops = {
    1217, 5733, 1061, 4798,  // left, top, right, bottom
    1.0, 1.0, 133, 133,  // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3, 0, 1, 1, 0  // max_fingers, max_touch, t5r2, semi_mt, is_button_pad
  };

  TestInterpreterWrapper wrapper(&interpreter, &hwprops);

  // HardwareState history should be initially cleared
  EXPECT_EQ(interpreter.prev_hwstate_.fingers, kNullFingers);
  EXPECT_EQ(interpreter.prev2_hwstate_.fingers, kNullFingers);

  // HardwareState history should not be cleared if interpreter enabled
  interpreter.interpreter_enabled_.val_ = 1;
  wrapper.SyncInterpret(&hs[0], NULL);
  EXPECT_TRUE(interpreter.prev_hwstate_.SameFingersAs(hs[0]));
  EXPECT_EQ(interpreter.prev2_hwstate_.fingers, kNullFingers);

  wrapper.SyncInterpret(&hs[1], NULL);
  EXPECT_TRUE(interpreter.prev_hwstate_.SameFingersAs(hs[1]));
  EXPECT_TRUE(interpreter.prev2_hwstate_.SameFingersAs(hs[0]));

  // HardwareState history should be cleared if interpreter disabled
  interpreter.interpreter_enabled_.val_ = 0;
  wrapper.SyncInterpret(&hs[0], NULL);
  EXPECT_EQ(interpreter.prev_hwstate_.fingers, kNullFingers);
  EXPECT_EQ(interpreter.prev2_hwstate_.fingers, kNullFingers);
}

const unsigned kWarpFlags = GESTURES_FINGER_WARP_X | GESTURES_FINGER_WARP_Y;

// Cr-48 tp firmware often reports the 'lifted' finger instead of the
// 'still present' finger for one packet following 2->1 finger transitions.
// This test simulates this, and tests that it doesn't generate motion.
TEST(Cr48ProfileSensorFilterInterpreterTest, TwoToOneJumpTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 65, 0, 3134, 2894, 67, 0},
    { 0, 0, 0, 0, 65, 0, 3132, 1891, 68, 0},
    { 0, 0, 0, 0, 65, 0, 3134, 2894, 68, 0},  // fs[0] position w/ fs[1] tid
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  };

  HardwareState hs[] = {
    { 0.500, 0, 2, 2, &fs[0], 0, 0, 0, 0 },  // fs[0] and fs[1]
    { 0.525, 0, 1, 1, &fs[2], 0, 0, 0, 0 },
    // fs[0] lifts, reported by fw w/ fs[1] tid
    { 0.550, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    // fw switches to reporting fs[1] position
    { 0.575, 0, 1, 1, &fs[1], 0, 0, 0, 0 },  // fw continues to report fs[1]
  };

  HardwareProperties hwprops = {
    1217, 5733, 1061, 4798,  // left, top, right, bottom
    1.0, 1.0, 133, 133,  // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3, 0, 1, 1, 0  // max_fingers, max_touch, t5r2, semi_mt, is_button_pad
  };

  TestInterpreterWrapper wrapper(&interpreter, &hwprops);
  interpreter.interpreter_enabled_.val_ = 1;

  for (size_t i = 0; i < arraysize(hs); i++) {
    // No Warp at first
    wrapper.SyncInterpret(&hs[i], NULL);
    FingerState *fs = &hs[i].fingers[0];
    switch (i) {
    case 0:  // No warp 2-finger sample, and 3rd sample after 2->1
    case 3:
      EXPECT_NE(fs->flags & kWarpFlags, kWarpFlags);
      break;
    case 1:  // Warp on 2->1 and sample after 2->1
    case 2:
      EXPECT_EQ(fs->flags & kWarpFlags, kWarpFlags);
      break;
    }
    fs->flags &= ~kWarpFlags;
  }
}

// Cr-48 tp firmware often reports a 'merged' position for the first one or two
// packets following a 1->2 finger transitions.  This could lead to unwanted
// pointer motion during drumroll.
// This test uses a data from a feedback report, and tests that the WARP flag
// is set after 1->2.
TEST(Cr48ProfileSensorFilterInterpreterTest, OneToTwoJumpTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 88, 0, 3316, 2522, 55, 0 },  // 0
    { 0, 0, 0, 0, 88, 0, 3317, 2522, 55, 0 },  // 1
    { 0, 0, 0, 0, 88, 0, 3213, 4542, 55, 0 },  // 2
    { 0, 0, 0, 0, 88, 0, 3446, 2523, 56, 0 },
    { 0, 0, 0, 0, 88, 0, 3129, 4586, 55, 0 },  // 4
    { 0, 0, 0, 0, 88, 0, 3316, 2522, 56, 0 },
    { 0, 0, 0, 0, 88, 0, 3068, 4636, 55, 0 },  // 6
    { 0, 0, 0, 0, 88, 0, 3312, 2520, 56, 0 },
  };

  HardwareState hs[] = {
    // time, buttons, finger count, touch count, fingers
    { 167.663976, 0, 1, 1, &fs[0], 0, 0, 0, 0 },  // 0
    { 167.674964, 0, 1, 1, &fs[1], 0, 0, 0, 0 },  // 1
    { 167.700748, 0, 2, 2, &fs[2], 0, 0, 0, 0 },  // 2
    { 167.724575, 0, 2, 2, &fs[4], 0, 0, 0, 0 },  // 3
    { 167.749059, 0, 2, 2, &fs[6], 0, 0, 0, 0 },  // 4
  };

  HardwareProperties hwprops = {
    1217, 1061, 5733, 4798,  // left, top, right, bottom
    47, 65, 133, 133,  // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3,  // max_fingers, max_touch
    false, true, true, false  // t5r2, semi_mt, is_button_pad
  };

  TestInterpreterWrapper wrapper(&interpreter, &hwprops);
  interpreter.interpreter_enabled_.val_ = 1;

  for (size_t i = 0; i < arraysize(hs); i++) {
    wrapper.SyncInterpret(&hs[i], NULL);
    switch (i) {
    case 2:  // warp x & y for 2 hwstates after 1->2 on all fingers
    case 3:
      EXPECT_EQ(hs[i].fingers[0].flags & kWarpFlags, kWarpFlags);
      EXPECT_EQ(hs[i].fingers[1].flags & kWarpFlags, kWarpFlags);
      break;
    default:
      break;
    }
  }
}

TEST(Cr48ProfileSensorFilterInterpreterTest, WarpOnSwapTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 73, 0, 3226, 2614, 24, 0 },  // 0
    { 0, 0, 0, 0, 73, 0, 3264, 2637, 24, 0 },  // 1
    { 0, 0, 0, 0, 92, 0, 3176, 4444, 24, 0 },  // 2
    { 0, 0, 0, 0, 92, 0, 3332, 2669, 25, 0 },
    { 0, 0, 0, 0, 93, 0, 3172, 4443, 24, 0 },  // 4
    { 0, 0, 0, 0, 93, 0, 3272, 2725, 25, 0 },
    { 0, 0, 0, 0, 95, 0, 3101, 4443, 24, 0 },  // 6
    { 0, 0, 0, 0, 95, 0, 3273, 2759, 25, 0 },
    { 0, 0, 0, 0, 97, 0, 3043, 4443, 24, 0 },  // 8
    { 0, 0, 0, 0, 97, 0, 3268, 2763, 25, 0 },
    { 0, 0, 0, 0, 101, 0, 3204, 4443, 24, 0 },  // 10
    { 0, 0, 0, 0, 102, 0, 3200, 4443, 24, 0 },  // 11
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  };

  HardwareState hs[] = {
    // time, buttons, finger count, touch count, fingers
    { 76.673715, 0, 1, 1, &fs[0], 0, 0, 0, 0 },  // 0
    { 76.68619699999999, 0, 1, 1, &fs[1], 0, 0, 0, 0 },  // 1
    { 76.712234, 0, 2, 2, &fs[2], 0, 0, 0, 0 },  // 2
    { 76.736775, 0, 2, 2, &fs[4], 0, 0, 0, 0 },  // 3
    { 76.760614, 0, 2, 2, &fs[6], 0, 0, 0, 0 },  // 4
    { 76.785092, 0, 2, 2, &fs[8], 0, 0, 0, 0 },  // 5
    { 76.808168, 0, 1, 1, &fs[10], 0, 0, 0, 0 },  // 6
    { 76.820516, 0, 1, 1, &fs[11], 0, 0, 0, 0 },  // 7
  };

  HardwareProperties hwprops = {
    1217, 1061, 5733, 4798,  // left, top, right, bottom
    47, 65, 133, 133,  // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3,  // max_fingers, max_touch
    false, true, true, false  // t5r2, semi_mt, is_button_pad
  };

  TestInterpreterWrapper wrapper(&interpreter, &hwprops);
  interpreter.interpreter_enabled_.val_ = 1;
  interpreter.bounding_box_.val_ = 1;

  for (size_t i = 0; i < arraysize(hs); i++) {
    wrapper.SyncInterpret(&hs[i], NULL);
    // fs[0] crosses fs[1] in hs[5], so WARP_X must be set
    if (i == 5) {
      EXPECT_EQ(hs[i].finger_cnt, 2);
      EXPECT_NE(hs[i].fingers, reinterpret_cast<FingerState *>(NULL));
      EXPECT_NE(hs[i].fingers[0].flags & GESTURES_FINGER_WARP_X, 0);
    }
  }
}

TEST(Cr48ProfileSensorFilterInterpreterTest, SensorJumpTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 74, 0, 2874, 4106, 1098, 0 },  // 0
    { 0, 0, 0, 0, 74, 0, 3460, 2565, 1101, 0 },

    { 0, 0, 0, 0, 74, 0, 2874, 4106, 1098, 0 },  // 2
    { 0, 0, 0, 0, 74, 0, 3460, 2500, 1101, 0 },  // a small move in Y

    { 0, 0, 0, 0, 74, 0, 2874, 4034, 1098, 0 },  // 4
    { 0, 0, 0, 0, 74, 0, 3485, 1367, 1101, 0 },  // a big jump in Y

    { 0, 0, 0, 0, 74, 0, 2874, 4034, 1098, 0 },  // 6
    { 0, 0, 0, 0, 74, 0, 3485, 1667, 1101, 0 },  // a sensor jump in Y

  };

  HardwareState hs[] = {
    // time, buttons, finger count, touch count, fingers
    { 42004.644897, 0, 2, 2, &fs[0], 0, 0, 0, 0 },  // 0
    { 42004.669832, 0, 2, 2, &fs[2], 0, 0, 0, 0 },  // 1
    { 42004.689832, 0, 2, 2, &fs[4], 0, 0, 0, 0 },  // 2
    { 42004.709832, 0, 2, 2, &fs[6], 0, 0, 0, 0 },  // 3 (a sensor jump report)
  };

  HardwareProperties hwprops = {
    1217, 1061, 5733, 4798,  // left, top, right, bottom
    47, 65, 133, 133,  // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3,  // max_fingers, max_touch
    false, true, true, false  // t5r2, semi_mt, is_button_pad
  };

  TestInterpreterWrapper wrapper(&interpreter, &hwprops);
  interpreter.interpreter_enabled_.val_ = 1;
  interpreter.min_jump_distance_.val_ = 150.0;
  interpreter.max_jump_distance_.val_ = 910.0;

  for (size_t i = 0; i < arraysize(hs); i++) {
    wrapper.SyncInterpret(&hs[i], NULL);
    if (i == 3)
      EXPECT_TRUE(interpreter.sensor_jump_[1][1]);
    else
      EXPECT_FALSE(interpreter.sensor_jump_[1][1]);
  }
}

TEST(Cr48ProfileSensorFilterInterpreterTest, BigJumpTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 76, 0, 4172, 2421, 5803, 0 },  // 0
    { 0, 0, 0, 0, 76, 0, 2952, 2901, 5803, 0 },  // 1 (jump bottom-left)
    { 0, 0, 0, 0, 76, 0, 2952, 2908, 5803, 0 },  // 2 (expect a new tracking id)
    { 0, 0, 0, 0, 76, 0, 2952, 2914, 5803, 0 },  // 3

    { 0, 0, 0, 0, 76, 0, 4172, 2421, 5804, 0 },  // 4
    { 0, 0, 0, 0, 76, 0, 4152, 1901, 5804, 0 },  // 5 (jump up)
    { 0, 0, 0, 0, 76, 0, 4152, 1908, 5804, 0 },  // 6 (expect a new tracking id)
    { 0, 0, 0, 0, 76, 0, 4152, 1914, 5804, 0 },  // 7

    { 0, 0, 0, 0, 76, 0, 4172, 1921, 5805, 0 },  // 8
    { 0, 0, 0, 0, 76, 0, 3152, 1901, 5805, 0 },  // 9 (jump left)
    { 0, 0, 0, 0, 76, 0, 4152, 1908, 5805, 0 },  // 10 (expect the same id)
    { 0, 0, 0, 0, 76, 0, 4152, 1914, 5805, 0 },  // 11
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  };

  HardwareState hs[] = {
    // time, buttons, finger count, touch count, fingers
    { 95268.194898, 0, 1, 1, &fs[0], 0, 0, 0, 0 },  // 0
    { 95268.203728, 0, 1, 1, &fs[1], 0, 0, 0, 0 },  // 1 (jump bottom-left)
    { 95268.216215, 0, 1, 1, &fs[2], 0, 0, 0, 0 },  // 2
    { 95268.229053, 0, 1, 1, &fs[3], 0, 0, 0, 0 },  // 3
    { 95268.24309, 0, 0, 0, NULL, 0, 0, 0, 0 },  // 4

    { 95268.194898, 0, 1, 1, &fs[4], 0, 0, 0, 0 },  // 5
    { 95268.203728, 0, 1, 1, &fs[5], 0, 0, 0, 0 },  // 6  (jump up)
    { 95268.216215, 0, 1, 1, &fs[6], 0, 0, 0, 0 },  // 7
    { 95268.229053, 0, 1, 1, &fs[7], 0, 0, 0, 0 },  // 8
    { 95268.24309, 0, 0, 0, NULL, 0, 0, 0, 0 },  // 9

    { 95268.194898, 0, 1, 1, &fs[8], 0, 0, 0, 0 },  // 10
    { 95268.203728, 0, 1, 1, &fs[9], 0, 0, 0, 0 },  // 11 (jump left)
    { 95268.216215, 0, 1, 1, &fs[10], 0, 0, 0, 0 }, // 12 jump back, so the
                                                    // tracking id
    { 95268.229053, 0, 1, 1, &fs[11], 0, 0, 0, 0 },  // 13 should not be changed
  };

  HardwareProperties hwprops = {
    1217, 1061, 5733, 4798,  // left, top, right, bottom
    47, 65, 133, 133,  // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3,  // max_fingers, max_touch
    false, true, true, false  // t5r2, semi_mt, is_button_pad
  };

  TestInterpreterWrapper wrapper(&interpreter, &hwprops);
  interpreter.interpreter_enabled_.val_ = 1;
  interpreter.move_threshold_.val_ = 130.0;
  interpreter.jump_threshold_.val_ = 260.0;

  for (size_t i = 0; i < arraysize(hs); i++) {
    struct FingerState *current = &hs[i].fingers[0];
    // prev finger with the same input tracking id
    struct FingerState *prev = NULL;
    if (i > 0 && hs[i].finger_cnt != 0 && hs[i - 1].finger_cnt != 0)
      prev = &hs[i - 1].fingers[0];

    wrapper.SyncInterpret(&hs[i], NULL);

    // Check if we squash the jump
    if (i == 1 || i == 11)
      EXPECT_EQ(prev->position_x, current->position_x);
    if (i == 1 || i == 6)
      EXPECT_EQ(prev->position_y, current->position_y);

    // Check if the tracking id is changed for a jump.
    if (i == 2 || i == 7)
      EXPECT_NE(prev->tracking_id, current->tracking_id);
    else if (prev != NULL)
      EXPECT_EQ(prev->tracking_id, current->tracking_id);
  }
}

TEST(Cr48ProfileSensorFilterInterpreterTest, FastMoveTest) {
  Cr48ProfileSensorFilterInterpreterTestInterpreter* base_interpreter =
      new Cr48ProfileSensorFilterInterpreterTestInterpreter;
  Cr48ProfileSensorFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 79, 0, 3481, 1910, 5824, 0 },  // 0
    { 0, 0, 0, 0, 79, 0, 3497, 2079, 5824, 0 },  // 1  fast move starts here
    { 0, 0, 0, 0, 79, 0, 3503, 2339, 5824, 0 },  // 2
    { 0, 0, 0, 0, 55, 0, 3534, 2742, 5824, 0 },  // 3
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  };

  HardwareState hs[] = {
    // time, buttons, finger count, touch count, fingers
    { 95268.194898, 0, 1, 1, &fs[0], 0, 0, 0, 0 },  // 0
    { 95268.203728, 0, 1, 1, &fs[1], 0, 0, 0, 0 },  // 1
    { 95268.216215, 0, 1, 1, &fs[2], 0, 0, 0, 0 },  // 2
    { 95268.229053, 0, 1, 1, &fs[3], 0, 0, 0, 0 },  // 3
  };

  HardwareProperties hwprops = {
    1217, 1061, 5733, 4798,  // left, top, right, bottom
    47, 65, 133, 133,  // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3,  // max_fingers, max_touch
    false, true, true, false  // t5r2, semi_mt, is_button_pad
  };

  TestInterpreterWrapper wrapper(&interpreter, &hwprops);
  interpreter.interpreter_enabled_.val_ = 1;
  interpreter.move_threshold_.val_ = 130.0;
  interpreter.jump_threshold_.val_ = 260.0;

  for (size_t i = 0; i < arraysize(hs); i++) {
    wrapper.SyncInterpret(&hs[i], NULL);
    struct FingerState *current = &fs[i];
    // Make sure the tracking ids are all the same.
    if (i > 0)
      EXPECT_EQ(fs[i - 1].tracking_id, current->tracking_id);
  }
}
}  // namespace gestures
