// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <gtest/gtest.h>  // for FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/set.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_FLING_STOP_FILTER_INTERPRETER_H_
#define GESTURES_FLING_STOP_FILTER_INTERPRETER_H_

namespace gestures {

// This interpreter generates the fling-stop messages when new fingers
// arrive on the pad.

class FlingStopFilterInterpreter : public FilterInterpreter {
  FRIEND_TEST(FlingStopFilterInterpreterTest, SimpleTest);
 public:
  // Takes ownership of |next|:
  FlingStopFilterInterpreter(PropRegistry* prop_reg, Interpreter* next,
                             Tracer* tracer);
  virtual ~FlingStopFilterInterpreter() {}

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

  virtual void HandleTimerImpl(stime_t now, stime_t* timeout);

  virtual void ConsumeGesture(const Gesture& gesture);

 private:
  // May override an outgoing gesture with a fling stop gesture.
  bool NeedsExtraTime(const HardwareState& hwstate) const;
  void UpdateFlingStopDeadline(const HardwareState& hwstate);
  stime_t SetNextDeadlineAndReturnTimeoutVal(stime_t now, stime_t next_timeout);

  // Has the deadline has already been extended once
  bool already_extended_;

  // Which tracking id's were on the pad at the last fling
  set<short, kMaxFingers> fingers_present_for_last_fling_;

  // tracking id's of the last hardware state
  set<short, kMaxFingers> fingers_of_last_hwstate_;

  // touch_cnt from previously input HardwareState.
  short prev_touch_cnt_;
  // timestamp from previous input HardwareState.
  stime_t prev_timestamp_;
  // Result to pass out.
  Gesture result_;

  // When we should send fling-stop, or 0.0 if not set.
  stime_t fling_stop_deadline_;
  // When we need to call HandlerTimer on next_, or 0.0 if no outstanding timer.
  stime_t next_timer_deadline_;

  // How long to wait when new fingers arrive (and possibly scroll), before
  // halting fling
  DoubleProperty fling_stop_timeout_;
  // How much extra time to add if it looks likely to be the start of a scroll
  DoubleProperty fling_stop_extra_delay_;
};

}  // namespace gestures

#endif  // GESTURES_FLING_STOP_FILTER_INTERPRETER_H_
