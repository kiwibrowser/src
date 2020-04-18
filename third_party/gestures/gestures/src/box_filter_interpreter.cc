// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/box_filter_interpreter.h"

#include "gestures/include/macros.h"
#include "gestures/include/tracer.h"
#include "gestures/include/util.h"

namespace gestures {

BoxFilterInterpreter::BoxFilterInterpreter(PropRegistry* prop_reg,
                                           Interpreter* next,
                                           Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      box_width_(prop_reg, "Box Width", 0.0),
      box_height_(prop_reg, "Box Height", 0.0) {
  InitName();
}

void BoxFilterInterpreter::SyncInterpretImpl(HardwareState* hwstate,
                                             stime_t* timeout) {
  if (box_width_.val_ == 0.0 && box_height_.val_ == 0.0) {
    next_->SyncInterpret(hwstate, timeout);
    return;
  }
  RemoveMissingIdsFromMap(&previous_output_, *hwstate);

  const float kHalfWidth = box_width_.val_ * 0.5;
  const float kHalfHeight = box_height_.val_ * 0.5;

  for (size_t i = 0; i < hwstate->finger_cnt; i++) {
    FingerState& fs = hwstate->fingers[i];
    // If it's new, pass it through
    if (!MapContainsKey(previous_output_, fs.tracking_id))
      continue;
    const FingerState& prev_out = previous_output_[fs.tracking_id];
    float FingerState::*fields[] = { &FingerState::position_x,
                                     &FingerState::position_y };
    const float kBounds[] = { kHalfWidth, kHalfHeight };
    unsigned warp[] = { GESTURES_FINGER_WARP_X_MOVE,
                        GESTURES_FINGER_WARP_Y_MOVE };
    for (size_t f_idx = 0; f_idx < arraysize(fields); f_idx++) {
      if (fs.flags & warp[f_idx])  // If warping, just move to the new point
        continue;
      float FingerState::*field = fields[f_idx];
      float prev_out_val = prev_out.*field;
      float val = fs.*field;
      float bound = kBounds[f_idx];

      if (prev_out_val - bound < val && val < prev_out_val + bound) {
        // keep box in place
        fs.*field = prev_out_val;
      } else if (val > prev_out_val) {
        fs.*field -= bound;
      } else {
        fs.*field += bound;
      }
    }
  }

  for (size_t i = 0; i < hwstate->finger_cnt; i++)
    previous_output_[hwstate->fingers[i].tracking_id] = hwstate->fingers[i];

  next_->SyncInterpret(hwstate, timeout);
}

}  // namespace gestures
