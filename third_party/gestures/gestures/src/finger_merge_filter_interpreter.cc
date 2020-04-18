// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/finger_merge_filter_interpreter.h"

#include <cmath>

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/gestures.h"
#include "gestures/include/logging.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

namespace gestures {

FingerMergeFilterInterpreter::FingerMergeFilterInterpreter(
    PropRegistry* prop_reg, Interpreter* next, Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      finger_merge_filter_enable_(prop_reg,
                                  "Finger Merge Filter Enabled", false),
      merge_distance_threshold_(prop_reg,
                                "Finger Merge Distance Thresh", 140.0),
      max_pressure_threshold_(prop_reg,
                              "Finger Merge Maximum Pressure", 83.0),
      min_pressure_threshold_(prop_reg,
                              "Finger Merge Min Pressure", 51.0),
      min_major_threshold_(prop_reg,
                           "Finger Merge Minimum Touch Major", 280.0),
      merged_major_pressure_ratio_(prop_reg,
                                   "Merged Finger Touch Major Pressure Ratio",
                                   5.0),
      merged_major_threshold_(prop_reg,
                              "Merged Finger Touch Major Thresh", 380.0),
      x_jump_min_displacement_(prop_reg, "Merged Finger X Jump Min Disp", 6.0),
      x_jump_max_displacement_(prop_reg, "Merged Finger X Jump Max Disp", 9.0),
      suspicious_angle_min_displacement_(
          prop_reg, "Merged Finger Suspicious Angle Min Displacement", 7.0),
      max_x_move_(prop_reg, "Merged Finger Max X Move", 180.0),
      max_y_move_(prop_reg, "Merged Finger Max Y Move", 60.0),
      max_age_(prop_reg, "Merged Finger Max Age", 0.350) {
  InitName();
}

void FingerMergeFilterInterpreter::SyncInterpretImpl(HardwareState* hwstate,
                                                     stime_t* timeout) {
  if (finger_merge_filter_enable_.val_)
    UpdateFingerMergeState(*hwstate);
  next_->SyncInterpret(hwstate, timeout);
}

// Suspicious angle is between the 45 degree angle of going down and to the
// left and going straight to the left
bool FingerMergeFilterInterpreter::IsSuspiciousAngle(
    const FingerState& fs) const {
  // We consider initial contacts as suspicious:
  if (!MapContainsKey(start_info_, fs.tracking_id))
    return true;
  const Start& start_info = start_info_[fs.tracking_id];
  float dx = fs.position_x - start_info.position_x;
  float dy = fs.position_y - start_info.position_y;
  // All angles are suspicious at very low distance
  if (dx * dx + dy * dy <
      suspicious_angle_min_displacement_.val_ *
      suspicious_angle_min_displacement_.val_)
    return true;
  if (dx > 0 || dy < 0)
    return false;
  return dy <= -1.0 * dx;
}

void FingerMergeFilterInterpreter::UpdateFingerMergeState(
    const HardwareState& hwstate) {

  RemoveMissingIdsFromMap(&start_info_, hwstate);
  RemoveMissingIdsFromSet(&merge_tracking_ids_, hwstate);
  RemoveMissingIdsFromSet(&never_merge_ids_, hwstate);
  RemoveMissingIdsFromMap(&prev_x_displacement_, hwstate);
  RemoveMissingIdsFromMap(&prev2_x_displacement_, hwstate);

  // Append GESTURES_FINGER_MERGE flag for close fingers and
  // fingers marked with the same flag previously
  for (short i = 0; i < hwstate.finger_cnt; i++) {
    FingerState *fs = hwstate.fingers;

    // If it's a new contact, add the initial info
    if (!MapContainsKey(start_info_, fs[i].tracking_id)) {
      Start start_info = { fs[i].position_x,
                           fs[i].position_y,
                           hwstate.timestamp };
      start_info_[fs[i].tracking_id] = start_info;
    }

    if (SetContainsValue(never_merge_ids_, fs[i].tracking_id))
      continue;
    if (SetContainsValue(merge_tracking_ids_, fs[i].tracking_id))
      fs[i].flags |= GESTURES_FINGER_MERGE;
    for (short j = i + 1; j < hwstate.finger_cnt; j++) {
      float xfd = fabsf(fs[i].position_x - fs[j].position_x);
      float yfd = fabsf(fs[i].position_y - fs[j].position_y);
      if (xfd < merge_distance_threshold_.val_ &&
          yfd < merge_distance_threshold_.val_) {
        fs[i].flags |= GESTURES_FINGER_MERGE;
        fs[j].flags |= GESTURES_FINGER_MERGE;
        merge_tracking_ids_.insert(fs[i].tracking_id);
        merge_tracking_ids_.insert(fs[j].tracking_id);
      }
    }
  }

  // Detect if there is a merged finger
  for (short i = 0; i < hwstate.finger_cnt; i++) {
    FingerState *fs = &hwstate.fingers[i];

    if (SetContainsValue(never_merge_ids_, fs->tracking_id))
      continue;

    // If exceeded some thresholds, not a merged finger
    const Start& start_info = start_info_[fs->tracking_id];
    float dx = fs->position_x - start_info.position_x;
    float dy = fs->position_y - start_info.position_y;
    stime_t dt = hwstate.timestamp - start_info.start_time;
    if (dx > max_x_move_.val_ || dy > max_y_move_.val_ || dt > max_age_.val_) {
      merge_tracking_ids_.erase(fs->tracking_id);
      never_merge_ids_.insert(fs->tracking_id);
      fs->flags &= ~GESTURES_FINGER_MERGE;
    }

    if (MapContainsKey(prev2_x_displacement_, fs->tracking_id)) {
      float displacement =
          fabsf(fs->position_x - start_info_[fs->tracking_id].position_x);
      float prev_disp = prev_x_displacement_[fs->tracking_id];
      float prev2_disp = prev2_x_displacement_[fs->tracking_id];
      if (prev2_disp <= x_jump_min_displacement_.val_ &&
          prev_disp > x_jump_min_displacement_.val_ &&
          displacement > prev_disp) {
        if (displacement < x_jump_max_displacement_.val_) {
          merge_tracking_ids_.erase(fs->tracking_id);
          never_merge_ids_.insert(fs->tracking_id);
          fs->flags &= ~GESTURES_FINGER_MERGE;
        }
      }
    }

    if (SetContainsValue(never_merge_ids_, fs->tracking_id))
      continue;

    // Basic criteria of a merged finger:
    //   - large touch major
    //   - small pressure value
    //   - good angle relative to start
    if (fs->flags & GESTURES_FINGER_MERGE ||
        fs->touch_major < min_major_threshold_.val_ ||
        fs->pressure > max_pressure_threshold_.val_ ||
        fs->pressure < min_pressure_threshold_.val_ ||
        !IsSuspiciousAngle(*fs))
      continue;


    // Filter out most false positive cases
    if (fs->touch_major > merged_major_pressure_ratio_.val_ * fs->pressure ||
        fs->touch_major > merged_major_threshold_.val_) {
      merge_tracking_ids_.insert(fs->tracking_id);
      fs->flags |= GESTURES_FINGER_MERGE;
    }
  }

  // Fill prev_x_displacement_
  prev2_x_displacement_ = prev_x_displacement_;
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    FingerState *fs = &hwstate.fingers[i];
    prev_x_displacement_[fs->tracking_id] =
        fabsf(start_info_[fs->tracking_id].position_x - fs->position_x);
  }
}

}
