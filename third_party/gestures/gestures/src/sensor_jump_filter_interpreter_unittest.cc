// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <deque>
#include <math.h>
#include <vector>
#include <utility>

#include <gtest/gtest.h>

#include "gestures/include/gestures.h"
#include "gestures/include/sensor_jump_filter_interpreter.h"
#include "gestures/include/unittest_util.h"

using std::deque;
using std::make_pair;
using std::pair;
using std::vector;

namespace gestures {

class SensorJumpFilterInterpreterTest : public ::testing::Test {};

class SensorJumpFilterInterpreterTestInterpreter : public Interpreter {
 public:
  SensorJumpFilterInterpreterTestInterpreter()
      : Interpreter(NULL, NULL, false),
        handle_timer_called_(false),
        expected_finger_cnt_(-1) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    if (expected_finger_cnt_ >= 0) {
      EXPECT_NE(static_cast<HardwareState*>(NULL), hwstate);
      EXPECT_EQ(1, hwstate->finger_cnt);
      prev_ = hwstate->fingers[0];
    }
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {
    handle_timer_called_ = true;
  }

  FingerState prev_;
  bool handle_timer_called_;
  short expected_finger_cnt_;
};

struct InputAndExpectedWarp {
  float val;
  bool warp;
};

TEST(SensorJumpFilterInterpreterTest, SimpleTest) {
  SensorJumpFilterInterpreterTestInterpreter* base_interpreter =
      new SensorJumpFilterInterpreterTestInterpreter;
  SensorJumpFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  base_interpreter->expected_finger_cnt_ = 1;
  interpreter.enabled_.val_ = 1;

  HardwareProperties hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    1, 1,  // x res (pixels/mm), y res (pixels/mm)
    1, 1,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    5, 5,  // max fingers, max_touch
    0, 0, 1, 0  // t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(&interpreter, &hwprops);

  EXPECT_FALSE(base_interpreter->handle_timer_called_);
  wrapper.HandleTimer(0.0, NULL);
  EXPECT_TRUE(base_interpreter->handle_timer_called_);

  FingerState fs = { 0, 0, 0, 0, 1, 0, 3.0, 0.0, 1, 0 };
  HardwareState hs = { 0.0, 0, 1, 1, &fs, 0, 0, 0, 0 };

  InputAndExpectedWarp data[] = {
    { 3.0, false },
    { 4.0, false },
    { 3.0, true },  // switch direction
    { 4.0, true },  // switch direction
    { 5.0, true },  // prev was flagged
    { 6.0, false },
    { 6.1, false },
    { 7.1, true },  // suspicious
    { 17.1, false },  // very large--okay
  };

  stime_t now = 0.0;
  const stime_t kTimeDelta = 0.01;
  for (size_t i = 0; i < arraysize(data); i++) {
    now += kTimeDelta;
    hs.timestamp = now;
    fs.flags = 0;
    fs.position_y = data[i].val;
    wrapper.SyncInterpret(&hs, NULL);
    const unsigned kFlags = GESTURES_FINGER_WARP_Y |
        GESTURES_FINGER_WARP_Y_TAP_MOVE |
        GESTURES_FINGER_WARP_TELEPORTATION;
    EXPECT_EQ(data[i].warp ? kFlags : 0, fs.flags) << "i=" << i;
  }
}

struct ActualLogInputs {
  stime_t timestamp;
  float x0, y0, p0;
  short id0;
  float x1, y1, p1;
  short id1;
};

// Real log with jumping fingers. Should only scroll in one direction
TEST(SensorJumpFilterInterpreterTest, ActualLogTest) {
  SensorJumpFilterInterpreterTestInterpreter* base_interpreter =
      new SensorJumpFilterInterpreterTestInterpreter;
  SensorJumpFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  interpreter.enabled_.val_ = 1;

  HardwareProperties hwprops = {
    0, 0, 106.666672, 68,  // left, top, right, bottom
    1, 1,  // x res (pixels/mm), y res (pixels/mm)
    25.4, 25.4,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    15, 5,  // max fingers, max_touch,
    0, 0, 1, 0  // t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(&interpreter, &hwprops);

  ActualLogInputs inputs[] = {
    { 19.554, 56.666, 59.400, 118.201, 6, 35.083, 58.200, 118.201, 7 },
    { 19.566, 56.583, 59.299, 118.201, 6, 35.083, 58.100, 118.201, 7 },
    { 19.578, 56.583, 59.100, 118.201, 6, 35.000, 58.100, 118.201, 7 },
    { 19.590, 56.500, 58.900, 118.201, 6, 34.750, 55.100, 116.261, 7 },
    { 19.601, 56.416, 58.799, 118.201, 6, 34.666, 54.900, 118.201, 7 },
    { 19.613, 56.333, 58.700, 118.201, 6, 34.666, 54.600, 118.201, 7 },
    { 19.625, 56.250, 58.500, 118.201, 6, 34.583, 54.500, 118.201, 7 },
    { 19.637, 56.166, 58.400, 118.201, 6, 34.583, 54.200, 118.201, 7 },
    { 19.648, 56.166, 58.299, 118.201, 6, 34.416, 53.900, 118.201, 7 },
    { 19.660, 56.083, 58.100, 118.201, 6, 34.416, 53.700, 118.201, 7 },
    { 19.672, 56.083, 58.100, 118.201, 6, 34.333, 53.500, 118.201, 7 },
    { 19.684, 56.083, 58.000, 118.201, 6, 34.250, 53.400, 118.201, 7 },
    { 19.696, 56.000, 57.900, 118.201, 6, 34.166, 53.200, 118.201, 7 },
    { 19.708, 55.916, 57.799, 118.201, 6, 34.166, 53.100, 118.201, 7 },
    { 19.720, 55.916, 57.799, 118.201, 6, 34.166, 53.000, 118.201, 7 },
    { 19.732, 55.916, 57.799, 118.201, 6, 34.083, 52.799, 118.201, 7 },
    { 19.744, 55.916, 57.799, 118.201, 6, 34.083, 52.700, 118.201, 7 },
    { 19.756, 55.833, 57.700, 118.201, 6, 34.000, 52.600, 118.201, 7 },
    { 19.768, 55.833, 57.500, 118.201, 6, 34.000, 52.500, 118.201, 7 },
    { 19.779, 55.333, 56.000, 118.201, 6, 34.000, 52.700, 118.201, 7 },
    { 19.791, 55.333, 55.900, 118.201, 6, 34.000, 52.700, 118.201, 7 },
    { 19.801, 55.333, 55.799, 118.201, 6, 34.000, 52.600, 118.201, 7 },
    { 19.813, 55.333, 55.400, 118.201, 6, 33.833, 52.200, 118.201, 7 },
    { 19.824, 55.333, 55.299, 118.201, 6, 33.833, 52.000, 118.201, 7 },
    { 19.835, 55.333, 55.200, 118.201, 6, 33.833, 51.799, 118.201, 7 },
    { 19.846, 55.333, 55.000, 118.201, 6, 33.750, 51.500, 118.201, 7 },
    { 19.857, 55.333, 54.700, 118.201, 6, 33.750, 51.200, 118.201, 7 },
    { 19.868, 55.333, 54.500, 118.201, 6, 33.750, 50.600, 118.201, 7 },
    { 19.880, 55.250, 54.299, 118.201, 6, 33.666, 50.500, 118.201, 7 },
    { 19.891, 55.250, 54.299, 118.201, 6, 33.666, 50.299, 118.201, 7 },
    { 19.902, 55.166, 54.100, 118.201, 6, 33.666, 50.100, 118.201, 7 },
    { 19.913, 55.166, 53.900, 116.261, 6, 33.666, 49.900, 118.201, 7 },
    { 19.924, 55.083, 53.900, 116.261, 6, 33.666, 49.799, 118.201, 7 },
    { 19.935, 55.083, 53.700, 116.261, 6, 33.583, 49.500, 118.201, 7 },
    { 19.947, 55.083, 53.500, 112.380, 6, 33.583, 49.299, 118.201, 7 },
    { 19.958, 55.083, 53.299, 110.439, 6, 33.583, 49.100, 118.201, 7 },
    { 19.969, 55.083, 53.100, 106.559, 6, 33.500, 48.900, 118.201, 7 },
    { 19.980, 55.000, 52.900, 104.618, 6, 33.416, 48.799, 118.201, 7 },
    { 19.991, 55.000, 52.799, 102.678, 6, 33.416, 48.600, 118.201, 7 },
    { 20.002, 55.000, 52.600, 98.7977, 6, 33.416, 48.299, 118.201, 7 },
    { 20.013, 54.916, 52.500, 96.8573, 6, 33.416, 48.000, 118.201, 7 },
    { 20.025, 54.833, 52.299, 92.9766, 6, 33.416, 47.700, 118.201, 7 },
    { 20.036, 54.666, 50.299, 118.201, 6, 33.333, 47.400, 118.201, 7 },
    { 20.047, 54.666, 50.100, 118.201, 6, 33.250, 47.000, 118.201, 7 },
    { 20.058, 54.666, 49.700, 118.201, 6, 33.250, 46.799, 118.201, 7 },
    { 20.069, 54.500, 49.299, 118.201, 6, 33.166, 46.299, 118.201, 7 },
    { 20.080, 54.416, 49.000, 118.201, 6, 33.166, 46.100, 118.201, 7 },
    { 20.091, 54.416, 48.700, 118.201, 6, 33.083, 45.600, 118.201, 7 },
    { 20.102, 54.416, 48.600, 118.201, 6, 33.083, 45.299, 118.201, 7 },
    { 20.113, 54.416, 48.299, 118.201, 6, 33.000, 45.000, 118.201, 7 },
    { 20.124, 54.416, 48.100, 118.201, 6, 33.000, 44.700, 118.201, 7 },
    { 20.135, 54.333, 47.700, 118.201, 6, 32.916, 44.000, 118.201, 7 },
    { 20.147, 54.250, 47.500, 118.201, 6, 32.833, 43.600, 118.201, 7 },
    { 20.158, 54.250, 47.400, 114.320, 6, 32.833, 43.400, 118.201, 7 },
    { 20.169, 54.250, 47.200, 112.380, 6, 32.750, 43.100, 118.201, 7 },
    { 20.180, 54.250, 47.000, 108.499, 6, 32.750, 42.799, 118.201, 7 },
    { 20.191, 54.250, 46.799, 106.559, 6, 32.666, 42.600, 118.201, 7 },
    { 20.202, 54.250, 46.600, 102.678, 6, 32.583, 42.299, 118.201, 7 },
    { 20.213, 54.166, 46.400, 98.7977, 6, 32.583, 42.000, 118.201, 7 },
    { 20.224, 54.000, 44.400, 118.201, 6, 32.583, 41.700, 118.201, 7 },
    { 20.235, 53.916, 44.000, 118.201, 6, 32.416, 41.200, 118.201, 7 },
    { 20.246, 53.833, 43.600, 118.201, 6, 32.416, 40.900, 118.201, 7 },
    { 20.257, 53.833, 43.299, 118.201, 6, 32.416, 40.500, 118.201, 7 },
    { 20.268, 53.750, 43.000, 118.201, 6, 32.416, 40.200, 118.201, 7 },
    { 20.280, 53.750, 42.799, 118.201, 6, 32.333, 40.000, 118.201, 7 },
    { 20.291, 53.750, 42.500, 118.201, 6, 32.333, 39.600, 118.201, 7 },
    { 20.302, 53.750, 42.400, 118.201, 6, 32.250, 39.200, 118.201, 7 },
    { 20.313, 53.666, 42.200, 118.201, 6, 32.166, 38.299, 118.201, 7 },
    { 20.324, 53.666, 41.900, 118.201, 6, 32.166, 38.000, 118.201, 7 },
    { 20.335, 53.666, 41.900, 118.201, 6, 32.166, 37.900, 118.201, 7 },
    { 20.347, 53.583, 41.500, 118.201, 6, 32.166, 37.700, 118.201, 7 },
    { 20.360, 53.500, 40.700, 182.233, 6, 31.666, 35.100, 118.201, 7 },
    { 20.374, 53.416, 40.200, 182.233, 6, 31.666, 35.000, 118.201, 7 },
    { 20.388, 53.416, 40.000, 182.233, 6, 31.666, 39.100, 118.201, 7 },
    { 20.401, 53.416, 39.700, 184.173, 6, 31.666, 38.900, 118.201, 7 },
    { 20.415, 53.333, 39.299, 184.173, 6, 31.666, 38.900, 118.201, 7 },
    { 20.429, 53.333, 38.900, 182.233, 6, 31.583, 38.700, 118.201, 7 },
    { 20.441, 52.666, 35.200, 118.201, 6, 31.666, 38.600, 118.201, 7 },
    { 20.453, 52.750, 35.100, 118.201, 6, 31.583, 38.500, 118.201, 7 },
    { 20.466, 52.666, 34.900, 118.201, 6, 31.666, 35.299, 189.995, 7 },
    { 20.480, 52.666, 34.799, 118.201, 6, 31.666, 35.000, 193.875, 7 },
    { 20.494, 52.666, 34.600, 118.201, 6, 31.666, 34.500, 191.935, 7 },
    { 20.507, 52.583, 34.500, 118.201, 6, 31.666, 34.299, 193.875, 7 },
    { 20.519, 52.583, 34.400, 118.201, 6, 31.333, 37.799, 118.201, 7 },
    { 20.533, 52.583, 34.299, 118.201, 6, 31.583, 33.700, 195.816, 7 },
    { 20.545, 52.666, 34.100, 118.201, 6, 31.000, 29.700, 118.201, 7 },
    { 20.557, 52.583, 34.000, 118.201, 6, 31.000, 29.400, 118.201, 7 },
    { 20.569, 52.583, 34.000, 118.201, 6, 31.000, 29.200, 118.201, 7 },
    { 20.581, 52.583, 34.000, 118.201, 6, 31.000, 29.100, 118.201, 7 },
    { 20.593, 52.583, 33.799, 118.201, 6, 30.916, 28.900, 118.201, 7 },
    { 20.605, 52.583, 33.799, 118.201, 6, 30.916, 28.800, 118.201, 7 },
  };

  FingerState fs[] = {
    { 0, 0, 0, 0, 1, 0, 3.0, 0.0, 1, 0 },
    { 0, 0, 0, 0, 1, 0, 3.0, 0.0, 1, 0 },
  };
  HardwareState hs = { 0.0, 0, 2, 2, &fs[0], 0, 0, 0, 0 };

  float prev_y_out[] = { 0.0, 0.0 };

  for (size_t i = 0; i < arraysize(inputs); i++) {
    const ActualLogInputs& input = inputs[i];
    hs.timestamp = input.timestamp;
    fs[0].position_x = input.x0;
    fs[0].position_y = input.y0;
    fs[0].pressure = input.p0;
    fs[0].tracking_id = input.id0;
    fs[1].position_x = input.x1;
    fs[1].position_y = input.y1;
    fs[1].pressure = input.p1;
    fs[1].tracking_id = input.id1;
    wrapper.SyncInterpret(&hs, NULL);
    if (i != 0) {  // can't do deltas with the first input
      float dy[] = { fs[0].position_y - prev_y_out[0],
                     fs[1].position_y - prev_y_out[1] };
      for (size_t j = 0; j < arraysize(dy); j++) {
        if (fs[j].flags & GESTURES_FINGER_WARP_Y)
          dy[j] = 0.0;
        EXPECT_LE(dy[j], 0.0) << "i=" << i << " j=" << j;
      }
    }
    prev_y_out[0] = fs[0].position_y;
    prev_y_out[1] = fs[1].position_y;
  }
}

}  // namespace gestures
