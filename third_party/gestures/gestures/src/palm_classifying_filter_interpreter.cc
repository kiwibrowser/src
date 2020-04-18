// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/palm_classifying_filter_interpreter.h"

#include "gestures/include/gestures.h"
#include "gestures/include/interpreter.h"
#include "gestures/include/tracer.h"
#include "gestures/include/util.h"

namespace gestures {

PalmClassifyingFilterInterpreter::PalmClassifyingFilterInterpreter(
    PropRegistry* prop_reg, Interpreter* next,
    Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      palm_pressure_(prop_reg, "Palm Pressure", 200.0),
      palm_width_(prop_reg, "Palm Width", 21.2),
      multi_palm_width_(prop_reg, "Multiple Palm Width", 75.0),
      fat_finger_pressure_ratio_(prop_reg, "Fat Finger Pressure Ratio", 1.4),
      fat_finger_width_ratio_(prop_reg, "Fat Finger Width Ratio", 1.3),
      fat_finger_min_dist_(prop_reg, "Fat Finger Min Move Distance", 15.0),
      palm_edge_min_width_(prop_reg, "Tap Exclusion Border Width", 8.0),
      palm_edge_width_(prop_reg, "Palm Edge Zone Width", 14.0),
      palm_edge_point_speed_(prop_reg, "Palm Edge Zone Min Point Speed", 100.0),
      palm_eval_timeout_(prop_reg, "Palm Eval Timeout", 0.1),
      palm_stationary_time_(prop_reg, "Palm Stationary Time", 2.0),
      palm_stationary_distance_(prop_reg, "Palm Stationary Distance", 4.0),
      palm_pointing_min_dist_(prop_reg,
                              "Palm Pointing Min Move Distance",
                              8.0),
      palm_pointing_max_reverse_dist_(prop_reg,
                                      "Palm Pointing Max Reverse Move Distance",
                                      0.3),
      palm_split_max_distance_(prop_reg, "Palm Split Maximum Distance", 4.0),
      filter_top_edge_(prop_reg, "Palm Filter Top Edge Enable", 0)
{
  InitName();
  requires_metrics_ = true;
}

void PalmClassifyingFilterInterpreter::SyncInterpretImpl(
    HardwareState* hwstate,
    stime_t* timeout) {
  FillOriginInfo(*hwstate);
  FillMaxPressureWidthInfo(*hwstate);
  UpdateDistanceInfo(*hwstate);
  UpdatePalmState(*hwstate);
  UpdatePalmFlags(hwstate);
  FillPrevInfo(*hwstate);
  if (next_.get())
    next_->SyncInterpret(hwstate, timeout);
}

void PalmClassifyingFilterInterpreter::FillOriginInfo(
    const HardwareState& hwstate) {
  RemoveMissingIdsFromMap(&origin_timestamps_, hwstate);
  RemoveMissingIdsFromMap(&origin_fingerstates_, hwstate);
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState& fs = hwstate.fingers[i];
    if (MapContainsKey(origin_timestamps_, fs.tracking_id))
      continue;
    origin_timestamps_[fs.tracking_id] = hwstate.timestamp;
    origin_fingerstates_[fs.tracking_id] = fs;
  }
}

void PalmClassifyingFilterInterpreter::FillPrevInfo(
    const HardwareState& hwstate) {
  RemoveMissingIdsFromMap(&prev_fingerstates_, hwstate);
  prev_time_ = hwstate.timestamp;
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState& fs = hwstate.fingers[i];
    prev_fingerstates_[fs.tracking_id] = fs;
  }
}

void PalmClassifyingFilterInterpreter::FillMaxPressureWidthInfo(
    const HardwareState& hwstate) {
  RemoveMissingIdsFromMap(&max_pressure_, hwstate);
  RemoveMissingIdsFromMap(&max_width_, hwstate);
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState& fs = hwstate.fingers[i];
    int id = fs.tracking_id;
    if (MapContainsKey(max_pressure_, id)) {
      if (fs.pressure > max_pressure_[id])
        max_pressure_[id] = fs.pressure;
      if (fs.touch_major > max_width_[id])
        max_width_[id] = fs.touch_major;
    } else {
      max_pressure_[id] = fs.pressure;
      max_width_[id] = fs.touch_major;
    }
  }
}

void PalmClassifyingFilterInterpreter::UpdateDistanceInfo(
    const HardwareState& hwstate) {
  RemoveMissingIdsFromMap(&distance_positive_[0], hwstate);
  RemoveMissingIdsFromMap(&distance_positive_[1], hwstate);
  RemoveMissingIdsFromMap(&distance_negative_[0], hwstate);
  RemoveMissingIdsFromMap(&distance_negative_[1], hwstate);
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState& fs = hwstate.fingers[i];
    int id = fs.tracking_id;
    if (MapContainsKey(prev_fingerstates_, id)) {
      float delta[2];
      delta[0] = fs.position_x - prev_fingerstates_[id].position_x;
      delta[1] = fs.position_y - prev_fingerstates_[id].position_y;
      for (int i = 0; i < 2; i++) {
        if (delta[i] > 0)
          distance_positive_[i][id] += delta[i];
        else
          distance_negative_[i][id] -= delta[i];
      }
    } else {
      distance_positive_[0][id] = 0;
      distance_positive_[1][id] = 0;
      distance_negative_[0][id] = 0;
      distance_negative_[1][id] = 0;
    }
  }
}

bool PalmClassifyingFilterInterpreter::FingerNearOtherFinger(
    const HardwareState& hwstate,
    size_t finger_idx) {
  const FingerState& fs = hwstate.fingers[finger_idx];
  for (int i = 0; i < hwstate.finger_cnt; ++i) {
    const FingerState& other_fs = hwstate.fingers[i];
    if (other_fs.tracking_id == fs.tracking_id)
      continue;
    bool close_enough_together =
        metrics_->CloseEnoughToGesture(Vector2(fs), Vector2(other_fs)) &&
        !SetContainsValue(palm_, other_fs.tracking_id);
    bool too_close_together = DistSq(fs, other_fs) <
        palm_split_max_distance_.val_ * palm_split_max_distance_.val_;
    if (close_enough_together && !too_close_together) {
      was_near_other_fingers_.insert(fs.tracking_id);
      return true;
    }
  }
  return false;
}

bool PalmClassifyingFilterInterpreter::FingerInPalmEnvelope(
    const FingerState& fs) {
  float limit = palm_edge_min_width_.val_ +
      (fs.pressure / palm_pressure_.val_) *
      (palm_edge_width_.val_ - palm_edge_min_width_.val_);
  return fs.position_x < limit ||
      fs.position_x > (hwprops_->right - limit);
}

bool PalmClassifyingFilterInterpreter::FingerInFilteredHorizontalEdge(
    const FingerState& fs) {
  return fs.position_y > (hwprops_->bottom - palm_edge_min_width_.val_) ||
      (filter_top_edge_.val_ && fs.position_y < palm_edge_min_width_.val_);
}

void PalmClassifyingFilterInterpreter::UpdatePalmState(
    const HardwareState& hwstate) {
  RemoveMissingIdsFromSet(&palm_, hwstate);
  RemoveMissingIdsFromMap(&pointing_, hwstate);
  RemoveMissingIdsFromSet(&non_stationary_palm_, hwstate);
  RemoveMissingIdsFromSet(&fingers_not_in_edge_, hwstate);
  RemoveMissingIdsFromSet(&was_near_other_fingers_, hwstate);

  // Some finger(s) just leaves, skip this update for stability
  if (prev_fingerstates_.size() > hwstate.finger_cnt)
    return;

  for (short i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState& fs = hwstate.fingers[i];
    if (!(FingerInPalmEnvelope(fs) || FingerInFilteredHorizontalEdge(fs)))
      fingers_not_in_edge_.insert(fs.tracking_id);
    // Mark anything over the palm thresh as a palm
    if (fs.pressure >= palm_pressure_.val_ ||
        fs.touch_major >= multi_palm_width_.val_) {
      palm_.insert(fs.tracking_id);
      pointing_.erase(fs.tracking_id);
      continue;
    }
  }

  if (hwstate.finger_cnt == 1 &&
      hwstate.fingers[0].touch_major >= palm_width_.val_) {
    palm_.insert(hwstate.fingers[0].tracking_id);
    pointing_.erase(hwstate.fingers[0].tracking_id);
  }

  const float kPalmStationaryDistSq =
      palm_stationary_distance_.val_ * palm_stationary_distance_.val_;
  const float kFatFingerMinDistSq =
      fat_finger_min_dist_.val_ * fat_finger_min_dist_.val_;
  const float kFatFingerMaxPressure =
      palm_pressure_.val_ * fat_finger_pressure_ratio_.val_;
  const float kFatFingerMaxWidth =
      palm_width_.val_ * fat_finger_width_ratio_.val_;

  for (short i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState& fs = hwstate.fingers[i];
    bool prev_palm = SetContainsValue(palm_, fs.tracking_id);
    bool prev_pointing = MapContainsKey(pointing_, fs.tracking_id);

    if (prev_palm) {
      // If the finger's pressure & width are more like a fat finger
      // and it has moved a lot, it might be a fat finger and remove
      // it from palm.
      float dist_sq = DistSq(origin_fingerstates_[fs.tracking_id], fs);
      if (max_pressure_[fs.tracking_id] <= kFatFingerMaxPressure &&
          max_width_[fs.tracking_id] <= kFatFingerMaxWidth &&
          dist_sq > kFatFingerMinDistSq) {
        palm_.erase(fs.tracking_id);
      } else {
        // Lock onto palm
        continue;
      }
    }

    // If the finger is recently placed, remove it from pointing/fingers.
    // If it's still looking like pointing, it'll get readded.
    if (FingerAge(fs.tracking_id, hwstate.timestamp) <
        palm_eval_timeout_.val_) {
      pointing_.erase(fs.tracking_id);

      prev_pointing = false;
    }
    // If another finger is close by, let this be pointing
    bool near_finger = FingerNearOtherFinger(hwstate, i);
    bool on_edge = FingerInPalmEnvelope(fs) ||
        FingerInFilteredHorizontalEdge(fs);
    if (!prev_pointing && (near_finger || !on_edge)) {
      unsigned reason = (near_finger ? kPointCloseToFinger : 0) |
          ((!on_edge) ? kPointNotInEdge : 0);
      pointing_[fs.tracking_id] = reason;
    }

    // Check if fingers that only move within palm envelope are pointing.
    int id = fs.tracking_id;
    float min_dist = palm_pointing_min_dist_.val_;
    float max_reverse_dist = palm_pointing_max_reverse_dist_.val_;

    // Ideally, we want to say that a finger is pointing if it moves only in
    // one direction significantly without zig-zag. But due to touch sensor's
    // inaccuratcy, we make the rule to be that a finger has to move in one
    // direction significantly with little move in the opposite direction.
    for (size_t j = 0; j < arraysize(distance_positive_); j++)
      if ((distance_positive_[j][id] >= min_dist &&
           distance_negative_[j][id] <= max_reverse_dist) ||
          (distance_positive_[j][id] <= max_reverse_dist &&
           distance_negative_[j][id] >= min_dist)) {
        pointing_[id] |= kPointMoving;
      }

    // However, if the contact has been stationary for a while since it
    // touched down, it is a palm. We track a potential palm closely for the
    // first amount of time to see if it fits this pattern.
    if (FingerAge(fs.tracking_id, prev_time_) >
        palm_stationary_time_.val_ ||
        SetContainsValue(non_stationary_palm_, fs.tracking_id)) {
      // Finger is too old to reconsider or is moving a lot
      continue;
    }
    if (DistSq(origin_fingerstates_[fs.tracking_id], fs) >
        kPalmStationaryDistSq || !(FingerInPalmEnvelope(fs) ||
                                   FingerInFilteredHorizontalEdge(fs))) {
      // Finger moving a lot or not in palm envelope; not a stationary palm.
      non_stationary_palm_.insert(fs.tracking_id);
      continue;
    }
    if (FingerAge(fs.tracking_id, prev_time_) <=
        palm_stationary_time_.val_ &&
        FingerAge(fs.tracking_id, hwstate.timestamp) >
        palm_stationary_time_.val_ &&
        !SetContainsValue(non_stationary_palm_, fs.tracking_id) &&
        !FingerNearOtherFinger(hwstate, i)) {
      // Enough time has passed. Make this stationary contact a palm.
      palm_.insert(fs.tracking_id);
      pointing_.erase(fs.tracking_id);
    }
  }
}

void PalmClassifyingFilterInterpreter::UpdatePalmFlags(HardwareState* hwstate) {
  for (short i = 0; i < hwstate->finger_cnt; i++) {
    FingerState* fs = &hwstate->fingers[i];
    if (SetContainsValue(palm_, fs->tracking_id)) {
      fs->flags |= GESTURES_FINGER_PALM;
    } else if (!MapContainsKey(pointing_, fs->tracking_id) &&
               !SetContainsValue(was_near_other_fingers_, fs->tracking_id)) {
      if (FingerInPalmEnvelope(*fs)) {
        fs->flags |= GESTURES_FINGER_PALM;
      } else if (FingerInFilteredHorizontalEdge(*fs)) {
        fs->flags |= (GESTURES_FINGER_WARP_X | GESTURES_FINGER_WARP_Y);
      }
    } else if (MapContainsKey(pointing_, fs->tracking_id) &&
               FingerInPalmEnvelope(*fs)) {
      fs->flags |= GESTURES_FINGER_POSSIBLE_PALM;
      if (pointing_[fs->tracking_id] == kPointCloseToFinger &&
          !FingerNearOtherFinger(*hwstate, i)) {
        // Finger was near another finger, but it's not anymore, and it was
        // only this other finger that caused it to point. Mark it w/ warp
        // until it moves sufficiently to have another reason to be
        // pointing.
        fs->flags |= (GESTURES_FINGER_WARP_X | GESTURES_FINGER_WARP_Y);
      }
    }
  }
}

stime_t PalmClassifyingFilterInterpreter::FingerAge(short finger_id,
                                                    stime_t now) const {
  if (!MapContainsKey(origin_timestamps_, finger_id)) {
    Err("Don't have record of finger age for finger %d", finger_id);
    return -1;
  }
  return now - origin_timestamps_[finger_id];
}

}  // namespace gestures
