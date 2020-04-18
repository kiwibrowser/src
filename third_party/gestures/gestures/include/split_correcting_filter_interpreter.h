// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>  // for FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/set.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_SPLIT_CORRECTING_FILTER_INTERPRETER_H_
#define GESTURES_SPLIT_CORRECTING_FILTER_INTERPRETER_H_

namespace gestures {

// This interepreter corrects problems that can occur with some touchpads.
// Currently, it corrects for the case where a large finger can erroneously
// "split" into two contacts. It works around this by looking for a contact
// to seemingly split into two, and fakes that the split didn't occur.

// This struct tracks an unmerged contact. By default the output and input
// IDs will be the same value, however after merge cycles, that may no longer
// be the case.
struct UnmergedContact {
  UnmergedContact() : input_id(-1) {}
  bool Valid() const { return input_id != -1; }
  void Invalidate() { input_id = -1; }
  short input_id;
  short output_id;
  float position_x;
  float position_y;
};

// Tracks two input contacts that are being combined into one output contact
// because we believe they are actually two parts of the same real contact.
struct MergedContact {
  MergedContact() : output_id(-1) {}
  bool Valid() const { return output_id != -1; }
  void Invalidate() { output_id = -1; }
  FingerState input_fingers[2];  // initial state
  short output_id;
};

class SplitCorrectingFilterInterpreter : public FilterInterpreter {
  FRIEND_TEST(SplitCorrectingFilterInterpreterTest, DistFromPointToLineTest);
 public:
  // Takes ownership of |next|:
  SplitCorrectingFilterInterpreter(PropRegistry* prop_reg, Interpreter* next,
                                   Tracer* tracer);
  virtual ~SplitCorrectingFilterInterpreter() {}

  void Enable() { enabled_.val_ = 1; }

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

 private:
  void RemoveMissingUnmergedContacts(const HardwareState& hwstate);
  void MergeFingers(const HardwareState& hwstate);
  void UnmergeFingers(const HardwareState& hwstate);
  void UpdateUnmergedLocations(const HardwareState& hwstate);

  // Given a line that goes through (x0, y0) and (x1, y1), and a separate
  // point, compute the square of the smallest distance from the point to the
  // line.
  static float DistSqFromPointToLine(float line_x_0, float line_y_0,
                                     float line_x_1, float line_y_1,
                                     float point_x, float point_y);

  // Based on merged_ and unmeged_, updates the current hwstate.
  void UpdateHwState(HardwareState* hwstate) const;
  // Tests to see if new_contact, when paired w/ existing_contact
  // are a good match for the unmerged contact, merge_recipient.
  // new_contact is the current state of the finger in merge_recipient.
  // Returns < 0 if this is not a good match, or an error value if it's good.
  // The smaller the error, the better.
  float AreMergePair(const FingerState& existing_contact,
                     const FingerState& new_contact,
                     const UnmergedContact& merge_recipient) const;

  void AppendMergedContact(const FingerState& input_a,
                           const FingerState& input_b,
                           short output_id);
  void AppendUnmergedContact(const FingerState& fs, short output_id);

  const UnmergedContact* FindUnmerged(short input_id) const;
  const MergedContact* FindMerged(short input_id) const;

  static void JoinFingerState(FingerState* in_out,
                              const FingerState& newfinger);
  static void RemoveFingerStateFromHardwareState(HardwareState* hs,
                                                 FingerState* fs);

  // Sets last_tracking_ids_ to the ids in the passed hwstate.
  void SetLastTrackingIds(const HardwareState& hwstate);

  // Dumps internal state and hwstate.
  void Dump(const HardwareState& hwstate) const;

  // We only enable on non-T5R2 pads
  BoolProperty enabled_;

  set<short, kMaxFingers> last_tracking_ids_;
  UnmergedContact unmerged_[kMaxFingers];
  MergedContact merged_[kMaxFingers / 2 + 1];

  // Contacts must be separated by less than this amount to be considered for
  // merging.
  DoubleProperty merge_max_separation_;
  // The most [mm] that a finger in a merged contact can move before we break
  // out and unmerge.
  DoubleProperty merge_max_movement_;
  // When merging, we expect that the two fingers that appear are on either
  // side of the old merged contact from last frame, and that the angle from
  // the old merged contact, to the new finger with the same id, to the new
  // finger with a new ID has a max angle. This angle then computes the max
  // ratio of: (distance from new ID point to line defined by old and new
  // contacts that have the same ID) / (length from new ID point to old ID point
  // in newer frame).
  DoubleProperty merge_max_ratio_;
};

}  // namespace gestures

#endif  // GESTURES_SPLIT_CORRECTING_FILTER_INTERPRETER_H_
