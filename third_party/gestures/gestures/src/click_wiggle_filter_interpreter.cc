// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/click_wiggle_filter_interpreter.h"

#include "gestures/include/gestures.h"
#include "gestures/include/interpreter.h"
#include "gestures/include/logging.h"
#include "gestures/include/tracer.h"

namespace gestures {

// Takes ownership of |next|:
ClickWiggleFilterInterpreter::ClickWiggleFilterInterpreter(
    PropRegistry* prop_reg, Interpreter* next, Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      button_edge_occurred_(0.0),
      prev_buttons_(0),
      wiggle_max_dist_(prop_reg, "Wiggle Max Distance", 5.5),
      wiggle_suppress_timeout_(prop_reg, "Wiggle Timeout", 0.075),
      wiggle_button_down_timeout_(prop_reg,
                                  "Wiggle Button Down Timeout",
                                  0.75),
      one_finger_click_wiggle_timeout_(prop_reg,
                                       "One Finger Click Wiggle Timeout",
                                       0.2) {
  InitName();
}

void ClickWiggleFilterInterpreter::SyncInterpretImpl(HardwareState* hwstate,
                                                     stime_t* timeout) {
  UpdateClickWiggle(*hwstate);
  SetWarpFlags(hwstate);

  // Update previous state
  prev_buttons_ = hwstate->buttons_down;
  RemoveMissingIdsFromMap(&prev_pressure_, *hwstate);
  for (size_t i = 0; i < hwstate->finger_cnt; i++) {
    const FingerState& fs = hwstate->fingers[i];
    prev_pressure_[fs.tracking_id] = fs.pressure;
  }

  next_->SyncInterpret(hwstate, timeout);
}

void ClickWiggleFilterInterpreter::UpdateClickWiggle(
    const HardwareState& hwstate) {
  // Removed outdated fingers from wiggle_recs_
  RemoveMissingIdsFromMap(&wiggle_recs_, hwstate);

  const bool button_down = hwstate.buttons_down & GESTURES_BUTTON_LEFT;
  const bool prev_button_down = prev_buttons_ & GESTURES_BUTTON_LEFT;
  const bool button_down_edge = button_down && !prev_button_down;
  const bool button_up_edge = !button_down && prev_button_down;

  if (button_down_edge || button_up_edge) {
    button_edge_occurred_ = hwstate.timestamp;
    size_t non_palm_count = 0;
    for (size_t i = 0; i < hwstate.finger_cnt; i++)
      if (!(hwstate.fingers[i].flags & (GESTURES_FINGER_PALM |
                                        GESTURES_FINGER_POSSIBLE_PALM)))
        non_palm_count++;
    button_edge_with_one_finger_ = (non_palm_count < 2);
  }

  // Update wiggle_recs_ for each current finger
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState& fs = hwstate.fingers[i];
    map<short, ClickWiggleRec, kMaxFingers>::iterator it =
        wiggle_recs_.find(fs.tracking_id);
    const bool new_finger = it == wiggle_recs_.end();

    if (button_down_edge || button_up_edge || new_finger) {
      stime_t timeout = button_down_edge ?
          wiggle_button_down_timeout_.val_ : wiggle_suppress_timeout_.val_;
      ClickWiggleRec rec = {
        fs.position_x,  // button down x
        fs.position_y,  // button down y
        hwstate.timestamp + timeout,  // unused during click down
        true  // suppress
      };
      wiggle_recs_[fs.tracking_id] = rec;
      continue;
    }

    // We have an existing finger
    ClickWiggleRec* rec = &(*it).second;

    if (!rec->suppress_)
      continue;  // It's already broken out of wiggle suppression

    float dx = fs.position_x - rec->x_;
    float dy = fs.position_y - rec->y_;
    if (dx * dx + dy * dy > wiggle_max_dist_.val_ * wiggle_max_dist_.val_) {
      // It's moved too much to be considered wiggle
      rec->suppress_ = false;
      continue;
    }

    if (hwstate.timestamp >= rec->began_press_suppression_) {
      // Too much time has passed to consider this wiggle
      rec->suppress_ = false;
      continue;
    }
  }
}

void ClickWiggleFilterInterpreter::SetWarpFlags(HardwareState* hwstate) const {
  if (button_edge_occurred_ != 0.0 &&
      button_edge_occurred_ < hwstate->timestamp &&
      button_edge_occurred_ + one_finger_click_wiggle_timeout_.val_ >
      hwstate->timestamp && button_edge_with_one_finger_) {
    for (size_t i = 0; i < hwstate->finger_cnt; i++)
      hwstate->fingers[i].flags |=
          (GESTURES_FINGER_WARP_X | GESTURES_FINGER_WARP_Y);
    // May as well return b/c already set warp on the only finger there is.
    return;
  }

  for (size_t i = 0; i < hwstate->finger_cnt; i++) {
    FingerState* fs = &hwstate->fingers[i];
    if (!MapContainsKey(wiggle_recs_, fs->tracking_id)) {
      Err("Missing finger in wiggle recs.");
      continue;
    }
    if (wiggle_recs_[fs->tracking_id].suppress_)
      fs->flags |= (GESTURES_FINGER_WARP_X | GESTURES_FINGER_WARP_Y);
  }
}

}  // namespace gestures
