// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/sensor_jump_filter_interpreter.h"

#include "gestures/include/tracer.h"
#include "gestures/include/util.h"

namespace gestures {

SensorJumpFilterInterpreter::SensorJumpFilterInterpreter(PropRegistry* prop_reg,
                                                         Interpreter* next,
                                                         Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      enabled_(prop_reg, "Sensor Jump Filter Enable", 0),
      min_warp_dist_non_move_(prop_reg, "Sensor Jump Min Dist Non-Move", 0.9),
      max_warp_dist_non_move_(prop_reg, "Sensor Jump Max Dist Non-Move", 7.5),
      similar_multiplier_non_move_(prop_reg,
                                   "Sensor Jump Similar Multiplier Non-Move",
                                   0.9),
      min_warp_dist_move_(prop_reg, "Sensor Jump Min Dist Move", 0.9),
      max_warp_dist_move_(prop_reg, "Sensor Jump Max Dist Move", 7.5),
      similar_multiplier_move_(prop_reg,
                               "Sensor Jump Similar Multiplier Move",
                               0.9),
      no_warp_min_dist_move_(prop_reg,
                             "Sensor Jump No Warp Min Dist Move",
                             0.21) {
  InitName();
}

void SensorJumpFilterInterpreter::SyncInterpretImpl(HardwareState* hwstate,
                                                        stime_t* timeout) {
  if (!enabled_.val_) {
    next_->SyncInterpret(hwstate, timeout);
    return;
  }

  RemoveMissingIdsFromMap(&previous_input_[0], *hwstate);
  RemoveMissingIdsFromMap(&previous_input_[1], *hwstate);
  RemoveMissingIdsFromSet(&first_flag_[0], *hwstate);
  RemoveMissingIdsFromSet(&first_flag_[1], *hwstate);
  RemoveMissingIdsFromSet(&first_flag_[2], *hwstate);
  RemoveMissingIdsFromSet(&first_flag_[3], *hwstate);

  map<short, FingerState, kMaxFingers> current_input;

  for (size_t i = 0; i < hwstate->finger_cnt; i++)
    current_input[hwstate->fingers[i].tracking_id] = hwstate->fingers[i];

  for (size_t i = 0; i < hwstate->finger_cnt; i++) {
    short tracking_id = hwstate->fingers[i].tracking_id;
    if (!MapContainsKey(previous_input_[1], tracking_id) ||
        !MapContainsKey(previous_input_[0], tracking_id))
      continue;
    FingerState* fs[] = {
      &hwstate->fingers[i],  // newest
      &previous_input_[0][tracking_id],
      &previous_input_[1][tracking_id],  // oldest
    };
    float FingerState::* const fields[] = { &FingerState::position_x,
                                            &FingerState::position_y,
                                            &FingerState::position_x,
                                            &FingerState::position_y };

    unsigned warp[] = { GESTURES_FINGER_WARP_X_NON_MOVE,
                        GESTURES_FINGER_WARP_Y_NON_MOVE,
                        GESTURES_FINGER_WARP_X_MOVE,
                        GESTURES_FINGER_WARP_Y_MOVE };

    for (size_t f_idx = 0; f_idx < arraysize(fields); f_idx++) {
      float FingerState::* const field = fields[f_idx];
      const float val[] = {
        fs[0]->*field,  // newest
        fs[1]->*field,
        fs[2]->*field,  // oldest
      };
      const float delta[] = {
        val[0] - val[1],  // newer
        val[1] - val[2],  // older
      };

      bool warp_move = (warp[f_idx] == GESTURES_FINGER_WARP_X_MOVE ||
                        warp[f_idx] == GESTURES_FINGER_WARP_Y_MOVE);
      float min_warp_dist = warp_move ? min_warp_dist_move_.val_ :
          min_warp_dist_non_move_.val_;
      float max_warp_dist = warp_move ? max_warp_dist_move_.val_ :
          max_warp_dist_non_move_.val_;
      float similar_multiplier = warp_move ? similar_multiplier_move_.val_ :
          similar_multiplier_non_move_.val_;

      const float kAllowableChange = fabsf(delta[1] * similar_multiplier);

      bool should_warp = false;
      bool should_store_flag = false;
      if (delta[0] * delta[1] < 0.0) {
        // switched direction
        // Don't mark direction change with small delta with WARP_*_MOVE.
        if (!warp_move || !(fabsf(delta[0]) < no_warp_min_dist_move_.val_ &&
                            fabsf(delta[1]) < no_warp_min_dist_move_.val_))
          should_store_flag = should_warp = true;
      } else if (fabsf(delta[0]) < min_warp_dist ||
                 fabsf(delta[0]) > max_warp_dist) {
        // acceptable movement
      } else if (fabsf(delta[0] - delta[1]) <= kAllowableChange) {
        if (SetContainsValue(first_flag_[f_idx], tracking_id)) {
          // Was flagged last time. Flag one more time
          should_warp = true;
        }
      } else {
        should_store_flag = should_warp = true;
      }
      if (should_warp) {
        fs[0]->flags |= (warp[f_idx] | GESTURES_FINGER_WARP_TELEPORTATION);
        // Warping moves here get tap warped, too
        if (warp_move) {
          fs[0]->flags |= warp[f_idx] == GESTURES_FINGER_WARP_X_MOVE ?
              GESTURES_FINGER_WARP_X_TAP_MOVE : GESTURES_FINGER_WARP_Y_TAP_MOVE;
        }
      }
      if (should_store_flag)
        first_flag_[f_idx].insert(tracking_id);
      else
        first_flag_[f_idx].erase(tracking_id);
    }
  }

  // Update previous input/output state
  previous_input_[1] = previous_input_[0];
  previous_input_[0] = current_input;

  next_->SyncInterpret(hwstate, timeout);
}

}  // namespace gestures
