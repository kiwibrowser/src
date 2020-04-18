// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <gtest/gtest.h>  // for FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/map.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_BOX_FILTER_INTERPRETER_H_
#define GESTURES_BOX_FILTER_INTERPRETER_H_

namespace gestures {

// This filter interpreter applies simple "box" algorithm to fingers as they
// pass through the filter. The purpose is to filter noisy input.
// The algorithm is: For each axis:
// - For each input point, compare X distance to previous output point for
//   the finger.
// - If the X distance is under box_width_ / 2, report the old location.
// - Report the new point shifted box_width_ / 2 toward the previous
//   output point.
// - Apply box_height_ to Y distance.
//
// The way to think about this is that there is a box around the previous
// output point with a width and height of box_width_. If a new point is
// within that box, report the old location (don't move the box). If the new
// point is outside, shift the box over to include the new point, then report
// the new center of the box.

class BoxFilterInterpreter : public FilterInterpreter, public PropertyDelegate {
  FRIEND_TEST(BoxFilterInterpreterTest, SimpleTest);
 public:
  // Takes ownership of |next|:
  BoxFilterInterpreter(PropRegistry* prop_reg, Interpreter* next,
                       Tracer* tracer);
  virtual ~BoxFilterInterpreter() {}

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

 private:
  DoubleProperty box_width_;
  DoubleProperty box_height_;

  map<short, FingerState, kMaxFingers> previous_output_;
};

}  // namespace gestures

#endif  // GESTURES_BOX_FILTER_INTERPRETER_H_
