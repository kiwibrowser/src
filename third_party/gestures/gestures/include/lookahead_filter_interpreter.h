// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <memory>

#include <gtest/gtest.h>  // For FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/list.h"
#include "gestures/include/map.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_LOOKAHEAD_FILTER_INTERPRETER_H_
#define GESTURES_LOOKAHEAD_FILTER_INTERPRETER_H_

namespace gestures {

class LookaheadFilterInterpreter : public FilterInterpreter {
  FRIEND_TEST(LookaheadFilterInterpreterTest, CyapaQuickTwoFingerMoveTest);
  FRIEND_TEST(LookaheadFilterInterpreterTest, DrumrollTest);
  FRIEND_TEST(LookaheadFilterInterpreterTest, InterpolateHwStateTest);
  FRIEND_TEST(LookaheadFilterInterpreterTest, InterpolateTest);
  FRIEND_TEST(LookaheadFilterInterpreterTest, InterpolationOverdueTest);
  FRIEND_TEST(LookaheadFilterInterpreterTest, NoTapSetTest);
  FRIEND_TEST(LookaheadFilterInterpreterTest, QuickMoveTest);
  FRIEND_TEST(LookaheadFilterInterpreterTest, QuickSwipeTest);
  FRIEND_TEST(LookaheadFilterInterpreterTest, SemiMtNoTrackingIdAssignmentTest);
  FRIEND_TEST(LookaheadFilterInterpreterTest, SimpleTest);
  FRIEND_TEST(LookaheadFilterInterpreterTest, SpuriousCallbackTest);
  FRIEND_TEST(LookaheadFilterInterpreterTest, VariableDelayTest);
 public:
  LookaheadFilterInterpreter(PropRegistry* prop_reg, Interpreter* next,
                             Tracer* tracer);
  virtual ~LookaheadFilterInterpreter() {}

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate,
                                 stime_t* timeout);

  virtual void HandleTimerImpl(stime_t now, stime_t* timeout);

  virtual void Initialize(const HardwareProperties* hwprops,
                          Metrics* metrics, MetricsProperties* mprops,
                          GestureConsumer* consumer);

 private:
  struct QState {
    QState();
    explicit QState(unsigned short max_fingers);

    // Deep copy of new_state to state_
    void set_state(const HardwareState& new_state);

    HardwareState state_;
    unsigned short max_fingers_;
    std::unique_ptr<FingerState[]> fs_;
    map<short, short, kMaxFingers> output_ids_;  // input tracking ids -> output

    stime_t due_;
    bool completed_;

    QState* next_;
    QState* prev_;
  };

  void LogVectors();

  // Produces a tapdown fling gesture if we just got a new hardware state
  // with a finger missing from the previous, or a null gesture otherwise.
  void TapDownOccurringGesture(stime_t now);

  // Looks at the most recent 2 states in the queue (one of which may have
  // already completed), and if they are separated by split_min_period_ time,
  // tries to insert an interpolated event in the middle.
  void AttemptInterpolation();

  // Reassigns tracking IDs, assigning them in such a way to avoid problems
  // of drumroll.
  void AssignTrackingIds();

  // For drumroll. Edits a QState node's fingerstate to have a new tracking id.
  void SeparateFinger(QState* node, FingerState* fs, short input_id);

  // Looks for a finger possibly lifting off the pad. If found, returns true.
  bool LiftoffJumpStarting(const HardwareState& hs,
                           const HardwareState& prev_hs,
                           const HardwareState& prev2_hs) const;

  // Returns a new tracking id for a contact.
  short NextTrackingId();

  // Interpolates first and second, storing the result into out.
  // first and second must have the same the same number of fingers and
  // have the same tracking_ids for all fingers.
  static void Interpolate(const HardwareState& first,
                          const HardwareState& second,
                          HardwareState* out);

  void UpdateInterpreterDue(stime_t new_interpreter_due,
                            stime_t now,
                            stime_t* timeout);
  void ConsumeGesture(const Gesture& gesture);

  stime_t ExtraVariableDelay() const;

  List<QState> queue_;
  List<QState> free_list_;

  // The last id assigned to a contact (part of drumroll suppression)
  short last_id_;

  unsigned short max_fingers_per_hwstate_;

  stime_t interpreter_due_;

  // We want to present time to next_ in a monotonically increasing manner,
  // so this keeps track of the most recent timestamp we've given next_.
  stime_t last_interpreted_time_;

  Gesture result_;

  DoubleProperty min_nonsuppress_speed_;
  DoubleProperty min_delay_;
  // On some platforms, min_delay_ is very small, and sometimes we would like
  // temporary extra delay to avoid problems, so we can in those cases add
  // a delay specified by max_delay_. It's okay for max_delay_ to be less
  // than min_delay_. In that case, it simply has no effect.
  DoubleProperty max_delay_;
  // If this much time passes between consecutive events, interpolate.
  DoubleProperty split_min_period_;
  // If set to false, tracking IDs are not reassigned
  BoolProperty drumroll_suppression_enable_;
  // If a contact appears to move faster than this, the drumroll detector may
  // consider it a new contact.
  DoubleProperty drumroll_speed_thresh_;
  // If one contact's speed is more than drumroll_max_speed_ratio_ times the
  // previous speed, the drumroll detector may consider it a new contact.
  DoubleProperty drumroll_max_speed_ratio_;
  // If during 3 consecutive HardwareState, one contact moves more than
  // quick_move_thresh_ distance along the same direction on either x or y
  // axis, both between the 1st and 2nd HardwareState, and the 2nd and 3rd
  // HardwareState, it is considered to be a quick move and the tracking ID
  // reassignment due to drumroll detection may get corrected.
  DoubleProperty quick_move_thresh_;
  // If we're going to drumroll-suppress a finger that is moving too much,
  // we abort said suppression if it's moving less than co_move_ratio_ *
  // distance of another non-drumroll-suppressed finger.
  DoubleProperty co_move_ratio_;
  // Temporary property to turn on/off the generation of TapDown gestures
  // (i.e., stop flinging gestures).
  BoolProperty suppress_immediate_tapdown_;
  // If we should add extra delay when we think a finger may be lifting off.
  BoolProperty delay_on_possible_liftoff_;
  // If looking for a possible liftoff-move, the speed a finger is moving
  // relative to the previous speed, such that it's a possible leave.
  DoubleProperty liftoff_speed_increase_threshold_;
};

}  // namespace gestures

#endif  // GESTURES_LOOKAHEAD_FILTER_INTERPRETER_H_
