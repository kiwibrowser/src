// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/t5r2_correcting_filter_interpreter.h"

namespace gestures {

// Takes ownership of |next|:
T5R2CorrectingFilterInterpreter::T5R2CorrectingFilterInterpreter(
    PropRegistry* prop_reg, Interpreter* next, Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      last_finger_cnt_(0),
      last_touch_cnt_(0),
      touch_cnt_correct_enabled_(prop_reg,
                                 "T5R2 Touch Count Correct Enabled", 1) {
  InitName();
}

void T5R2CorrectingFilterInterpreter::SyncInterpretImpl(
    HardwareState* hwstate,
    stime_t* timeout) {
  if (touch_cnt_correct_enabled_.val_ &&
      hwstate->finger_cnt == 0 && last_finger_cnt_ == 0 &&
      hwstate->touch_cnt != 0 && hwstate->touch_cnt == last_touch_cnt_) {
    hwstate->touch_cnt = 0;
  }
  last_touch_cnt_ = hwstate->touch_cnt;
  last_finger_cnt_ = hwstate->finger_cnt;
  next_->SyncInterpret(hwstate, timeout);
}

};  // namespace gestures
