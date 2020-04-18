// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/split_correcting_filter_interpreter.h"

#include <math.h>

#include "gestures/include/tracer.h"
#include "gestures/include/util.h"

namespace gestures {

// Takes ownership of |next|:
SplitCorrectingFilterInterpreter::SplitCorrectingFilterInterpreter(
    PropRegistry* prop_reg, Interpreter* next, Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      enabled_(prop_reg, "Split Corrector Enabled", 0),
      merge_max_separation_(prop_reg, "Split Merge Max Separation", 17.0),
      merge_max_movement_(prop_reg, "Split Merge Max Movement", 3.0),
      merge_max_ratio_(prop_reg, "Merge Max Ratio", sinf(DegToRad(19.0))) {
  InitName();
}

void SplitCorrectingFilterInterpreter::SyncInterpretImpl(
    HardwareState* hwstate,
    stime_t* timeout) {
  // Update internal state
  if (enabled_.val_) {
    RemoveMissingUnmergedContacts(*hwstate);
    MergeFingers(*hwstate);
    UnmergeFingers(*hwstate);
    UpdateUnmergedLocations(*hwstate);
    SetLastTrackingIds(*hwstate);

    // Use internal state to update hwstate
    UpdateHwState(hwstate);
  }
  next_->SyncInterpret(hwstate, timeout);
}

void SplitCorrectingFilterInterpreter::RemoveMissingUnmergedContacts(
    const HardwareState& hwstate) {
  for (UnmergedContact* it = unmerged_;
       it < &unmerged_[arraysize(unmerged_)] &&
           it->Valid();) {
    if (!hwstate.GetFingerState(it->input_id)) {
      // Erase this element
      std::copy(it + 1, &unmerged_[arraysize(unmerged_)], it);
      unmerged_[arraysize(unmerged_) - 1].Invalidate();
    } else {
      ++it;
    }
  }
}

void SplitCorrectingFilterInterpreter::MergeFingers(
    const HardwareState& hwstate) {
  set<const FingerState*, kMaxFingers> unused;
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    if (!SetContainsValue(last_tracking_ids_, hwstate.fingers[i].tracking_id))
      unused.insert(&hwstate.fingers[i]);
  }
  if (unused.empty())
    return;
  for (UnmergedContact* it = unmerged_; it->Valid();) {
    // Current state of the unmerged finger
    const FingerState* existing_contact = hwstate.GetFingerState(it->input_id);
    if (!existing_contact) {
      Err("How is existing_contact NULL?");
      return;
    }
    // try all fingers for possible merging
    float min_error = INFINITY;
    set<const FingerState*, kMaxFingers>::iterator min_error_it = unused.end();
    for (set<const FingerState*, kMaxFingers>::iterator unused_it =
             unused.begin(), e = unused.end(); unused_it != e; ++unused_it) {
      const FingerState* new_contact = *unused_it;
      if (new_contact == existing_contact)
        continue;
      float error = AreMergePair(*existing_contact, *new_contact, *it);
      if (error < 0)
        continue;
      if (error < min_error) {
        min_error = error;
        min_error_it = unused_it;
      }
    }
    if (min_error_it != unused.end()) {
      // we have a merge!
      AppendMergedContact(*existing_contact, *(*min_error_it), it->output_id);
      unused.erase(min_error_it);
      // Delete this UnmergedContact
      std::copy(it + 1, &unmerged_[arraysize(unmerged_)], it);
      unmerged_[arraysize(unmerged_) - 1].Invalidate();
      continue;
    }
    it++;
  }
  if (unused.empty())
    return;
  // Put the unused new fingers into the unmerged fingers
  // Find next slot
  UnmergedContact* it = unmerged_;
  for (; it->Valid() && it != &unmerged_[kMaxFingers]; ++it) {}
  for (set<const FingerState*, kMaxFingers>::iterator unused_it =
           unused.begin(), e = unused.end(); unused_it != e; ++unused_it) {
    if (it == &unmerged_[kMaxFingers]) {
      Err("How is there no space?");
      return;
    }
    const FingerState& fs = *(*unused_it);
    it->input_id = it->output_id = fs.tracking_id;
    it->position_x = fs.position_x;
    it->position_y = fs.position_y;
    it++;
  }
}

void SplitCorrectingFilterInterpreter::AppendMergedContact(
    const FingerState& input_a,
    const FingerState& input_b,
    short output_id) {
  for (size_t i = 0; i < arraysize(merged_); i++) {
    if (merged_[i].Valid())
      continue;
    merged_[i].input_fingers[0] = input_a;
    merged_[i].input_fingers[1] = input_b;
    merged_[i].output_id = output_id;
    return;
  }
  Err("No free merged contact?");
  return;
}

void SplitCorrectingFilterInterpreter::AppendUnmergedContact(
    const FingerState& fs, short output_id) {
  for (size_t i = 0; i < arraysize(unmerged_); i++) {
    if (unmerged_[i].Valid())
      continue;
    unmerged_[i].input_id = fs.tracking_id;
    unmerged_[i].output_id = output_id;
    unmerged_[i].position_x = fs.position_x;
    unmerged_[i].position_y = fs.position_y;
    return;
  }
  Err("No free unmerged contact?");
}

float SplitCorrectingFilterInterpreter::AreMergePair(
    const FingerState& existing_contact,
    const FingerState& new_contact,
    const UnmergedContact& merge_recipient) const {
  // Is it close enough to the old contact?
  const float kMaxSepSq =
      merge_max_separation_.val_ * merge_max_separation_.val_;
  float sep_sq = DistSq(new_contact, existing_contact);
  if (sep_sq > kMaxSepSq) {
    return -1;
  }
  // Does this new contact help?
  float existing_move_sq = DistSq(merge_recipient, existing_contact);
  float mid_x = (new_contact.position_x + existing_contact.position_x) * 0.5;
  float mid_y = (new_contact.position_y + existing_contact.position_y) * 0.5;
  float old_to_mid_dist_sq = DistSqXY(merge_recipient, mid_x, mid_y);
  if (old_to_mid_dist_sq < existing_move_sq)
    return old_to_mid_dist_sq;  // Return new distance; definite improvement

  // Check if the merge recipient is too far from new_contact
  float current_dist_sq = DistSq(existing_contact, new_contact);
  float new_to_reicpient_sq = DistSq(merge_recipient, new_contact);
  if (current_dist_sq < new_to_reicpient_sq)
    return -1;

  // Check if the new contact is, more or less, "along the line" from
  // existing contact through merge_recipient, and beyond.

  // Distance_sq from new_contact to the line that goes through merge_recipient
  // and existing_contact.
  const float orthogonal_dist_sq =
      DistSqFromPointToLine(merge_recipient.position_x,
                            merge_recipient.position_y,
                            existing_contact.position_x,
                            existing_contact.position_y,
                            new_contact.position_x,
                            new_contact.position_y);

  // Imagine a right-triangle like so:
  //                         /|(new point)
  //                      /   | <== orthogonal_dist
  // (existing_contact)/__m___|(right angle)
  //                    ^^^^^ line between existing_contact and merge_recipient
  // m = merge_recipient point.
  // We compute the maximum ratio of orthogonal_dist / hypotenuse length

  if (orthogonal_dist_sq <
      merge_max_ratio_.val_ * merge_max_ratio_.val_ * current_dist_sq)
    return old_to_mid_dist_sq;  // merge!

  return -1;  // no merge
}

// static
float SplitCorrectingFilterInterpreter::DistSqFromPointToLine(float line_x_0,
                                                              float line_y_0,
                                                              float line_x_1,
                                                              float line_y_1,
                                                              float point_x,
                                                              float point_y) {
  // Find general form (A*x + B*y + C = 0) of a line given two points.
  float line_a = line_y_0 - line_y_1;
  float line_b = line_x_1 - line_x_0;
  float line_c = line_x_0 * line_y_1 - line_y_0 * line_x_1;
  // Compute min distance from line to point_(x,y)
  float num = line_a * point_x + line_b * point_y + line_c;
  float den_sq = line_a * line_a + line_b * line_b;
  if (den_sq == 0.0)
    return 0.0;  // don't crash
  return num * num / den_sq;
}

void SplitCorrectingFilterInterpreter::UnmergeFingers(
    const HardwareState& hwstate) {
  const float kMaxSepSq =
      merge_max_separation_.val_ * merge_max_separation_.val_;
  const float kMaxMoveSq =
      merge_max_movement_.val_ * merge_max_movement_.val_;
  for (size_t i = 0; i < arraysize(merged_);) {
    MergedContact* mc = &merged_[i];
    if (!mc->Valid())
      break;
    const FingerState* first =
        hwstate.GetFingerState(mc->input_fingers[0].tracking_id);
    const FingerState* second =
        hwstate.GetFingerState(mc->input_fingers[1].tracking_id);
    if (first && second && DistSq(*first, *second) <= kMaxSepSq &&
        DistSq(*first, mc->input_fingers[0]) < kMaxMoveSq &&
        DistSq(*second, mc->input_fingers[1]) < kMaxMoveSq) {
      i++;
      continue;
    }
    if (first)
      AppendUnmergedContact(*first, mc->output_id);
    if (second)
      // For no good reason, if we have both first and second, we give
      // first the output id, thus it takes over for the merged finger
      AppendUnmergedContact(*second,
                            first ? second->tracking_id : mc->output_id);
    // Delete this element
    std::copy(&merged_[i + 1], &merged_[arraysize(merged_)], &merged_[i]);
    merged_[arraysize(merged_) - 1].Invalidate();
  }
}

void SplitCorrectingFilterInterpreter::UpdateHwState(
    HardwareState* hwstate) const {
  for (size_t i = 0; i < hwstate->finger_cnt; i++) {
    FingerState* fs = &hwstate->fingers[i];
    const UnmergedContact* unmerged = FindUnmerged(fs->tracking_id);
    if (unmerged && unmerged->Valid()) {
      // Easier case. Just update tracking id
      fs->tracking_id = unmerged->output_id;
      continue;
    }
    const MergedContact* merged = FindMerged(fs->tracking_id);
    if (merged && merged->Valid()) {
      short other_id = merged->input_fingers[0].tracking_id != fs->tracking_id ?
          merged->input_fingers[0].tracking_id :
          merged->input_fingers[1].tracking_id;
      FingerState* other_fs = hwstate->GetFingerState(other_id);
      if (!other_fs) {
        Err("Missing other finger state?");
        return;
      }
      JoinFingerState(fs, *other_fs);
      fs->tracking_id = merged->output_id;
      RemoveFingerStateFromHardwareState(hwstate, other_fs);
      continue;
    }
    Err("Neither unmerged nor merged?");
    return;
  }
  hwstate->touch_cnt = hwstate->finger_cnt;
}

const UnmergedContact* SplitCorrectingFilterInterpreter::FindUnmerged(
    short input_id) const {
  for (size_t i = 0; i < arraysize(unmerged_) && unmerged_[i].Valid(); i++)
    if (unmerged_[i].input_id == input_id)
      return &unmerged_[i];
  return NULL;
}

const MergedContact* SplitCorrectingFilterInterpreter::FindMerged(
    short input_id) const {
  for (size_t i = 0; i < arraysize(merged_) && merged_[i].Valid(); i++)
    if (merged_[i].input_fingers[0].tracking_id == input_id ||
        merged_[i].input_fingers[1].tracking_id == input_id)
      return &merged_[i];
  return NULL;
}

// static
void SplitCorrectingFilterInterpreter::JoinFingerState(
    FingerState* in_out, const FingerState& newfinger) {
  float FingerState::*fields[] = { &FingerState::touch_major,
                                   &FingerState::touch_minor,
                                   &FingerState::width_major,
                                   &FingerState::width_minor,
                                   &FingerState::pressure,
                                   &FingerState::orientation,
                                   &FingerState::position_x,
                                   &FingerState::position_y };
  for (size_t f_idx = 0; f_idx < arraysize(fields); f_idx++) {
    float FingerState::*field = fields[f_idx];
    in_out->*field = (in_out->*field + newfinger.*field) * 0.5;
  }
  in_out->flags |= newfinger.flags |
      GESTURES_FINGER_WARP_X |
      GESTURES_FINGER_WARP_Y;
}

// static
void SplitCorrectingFilterInterpreter::RemoveFingerStateFromHardwareState(
    HardwareState* hs,
    FingerState* fs) {
  std::copy(fs + 1, &hs->fingers[hs->finger_cnt], fs);
  hs->finger_cnt--;
}

void SplitCorrectingFilterInterpreter::SetLastTrackingIds(
    const HardwareState& hwstate) {
  last_tracking_ids_.clear();
  for (size_t i = 0; i < hwstate.finger_cnt; i++)
    last_tracking_ids_.insert(hwstate.fingers[i].tracking_id);
}

void SplitCorrectingFilterInterpreter::UpdateUnmergedLocations(
    const HardwareState& hwstate) {
  for (size_t i = 0; i < arraysize(unmerged_) && unmerged_[i].Valid(); i++) {
    const FingerState* fs = hwstate.GetFingerState(unmerged_[i].input_id);
    if (!fs) {
      Err("Missing finger state?");
      continue;
    }
    unmerged_[i].position_x = fs->position_x;
    unmerged_[i].position_y = fs->position_y;
  }
}

void SplitCorrectingFilterInterpreter::Dump(
    const HardwareState& hwstate) const {
  Log("Last Tracking IDs:");
  for (set<short, kMaxFingers>::const_iterator it = last_tracking_ids_.begin(),
           e = last_tracking_ids_.end(); it != e; ++it)
    Log("  %d", *it);
  Log("Unmerged:");
  for (size_t i = 0; i < arraysize(unmerged_); i++)
    Log("  %sin: %d out: %d x: %f y: %f",
        unmerged_[i].Valid() ? "" : "INV ",
        unmerged_[i].input_id,
        unmerged_[i].output_id,
        unmerged_[i].position_x,
        unmerged_[i].position_y);
  Log("Merged:");
  for (size_t i = 0; i < arraysize(merged_); i++)
    Log("  %sin: %d in: %d out: %d",
        merged_[i].Valid() ? "" : "INV ",
        merged_[i].input_fingers[0].tracking_id,
        merged_[i].input_fingers[1].tracking_id,
        merged_[i].output_id);
  Log("HW state IDs:");
  for (size_t i = 0; i < hwstate.finger_cnt; i++)
    Log("  %d", hwstate.fingers[i].tracking_id);
}

};  // namespace gestures
