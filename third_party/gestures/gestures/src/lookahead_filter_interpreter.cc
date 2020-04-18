// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/lookahead_filter_interpreter.h"

#include <algorithm>
#include <math.h>
#include <values.h>

#include "gestures/include/tracer.h"
#include "gestures/include/util.h"

using std::max;
using std::min;

namespace gestures {

namespace {
static const stime_t kMaxDelay = 0.09;  // 90ms
}

LookaheadFilterInterpreter::LookaheadFilterInterpreter(
    PropRegistry* prop_reg, Interpreter* next, Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      last_id_(0), max_fingers_per_hwstate_(0), interpreter_due_(-1.0),
      last_interpreted_time_(0.0),
      min_nonsuppress_speed_(prop_reg, "Input Queue Min Nonsuppression Speed",
                             200.0),
      min_delay_(prop_reg, "Input Queue Delay", 0.0),
      max_delay_(prop_reg, "Input Queue Max Delay", 0.017),
      split_min_period_(prop_reg, "Min Interpolate Period", 0.021),
      drumroll_suppression_enable_(prop_reg, "Drumroll Suppression Enable", 1),
      drumroll_speed_thresh_(prop_reg, "Drumroll Speed Thresh", 400.0),
      drumroll_max_speed_ratio_(prop_reg,
                                "Drumroll Max Speed Change Factor",
                                15.0),
      quick_move_thresh_(prop_reg, "Quick Move Distance Thresh", 3.0),
      co_move_ratio_(prop_reg, "Drumroll Co Move Ratio", 1.2),
      suppress_immediate_tapdown_(prop_reg, "Suppress Immediate Tapdown", 1),
      delay_on_possible_liftoff_(prop_reg, "Delay On Possible Liftoff", 0),
      liftoff_speed_increase_threshold_(prop_reg, "Liftoff Speed Factor", 5.0) {
  InitName();
}

void LookaheadFilterInterpreter::SyncInterpretImpl(HardwareState* hwstate,
                                                       stime_t* timeout) {
  // Push back into queue
  if (free_list_.Empty()) {
    Err("Can't accept new hwstate b/c we're out of nodes!");
    Err("Now: %f, interpreter_due_ %f", hwstate->timestamp, interpreter_due_);
    Err("Dump of queue:");
    for (QState* it = queue_.Begin(); it != queue_.End(); it = it->next_)
      Err("Due: %f%s", it->due_, it->completed_ ? " (c)" : "");
    return;
  }
  QState* node = free_list_.PopFront();
  node->set_state(*hwstate);
  double delay = max(0.0, min<stime_t>(kMaxDelay, min_delay_.val_));
  node->due_ = hwstate->timestamp + delay;
  node->completed_ = false;
  if (queue_.Empty())
    node->output_ids_.clear();
  else
    node->output_ids_ = queue_.Tail()->output_ids_;
  // At this point, if ExtraVariableDelay() > 0, queue_.Tail()->due_ may have
  // ExtraVariableDelay() applied, but node->due_ does not, yet.
  if (!queue_.Empty() &&
      (queue_.Tail()->due_ - node->due_ > ExtraVariableDelay())) {
    Err("Clock changed backwards. Clearing queue.");
    do {
      free_list_.PushBack(queue_.PopFront());
    } while (!queue_.Empty());
    interpreter_due_ = -1.0;
    last_interpreted_time_ = 0.0;
  }
  queue_.PushBack(node);
  AssignTrackingIds();
  AttemptInterpolation();
  UpdateInterpreterDue(interpreter_due_ < 0.0 ?
                       interpreter_due_ : interpreter_due_ + hwstate->timestamp,
                       hwstate->timestamp, timeout);
  HandleTimerImpl(hwstate->timestamp, timeout);
}

// Interpolates the two hardware states into out.
// out must have finger states allocated and pointed to already.
void LookaheadFilterInterpreter::Interpolate(const HardwareState& first,
                                             const HardwareState& second,
                                             HardwareState* out) {
  out->timestamp = (first.timestamp + second.timestamp) / 2.0;
  out->buttons_down = first.buttons_down;
  out->touch_cnt = first.touch_cnt;
  out->finger_cnt = first.finger_cnt;
  for (size_t i = 0; i < first.finger_cnt; i++) {
    const FingerState& older = first.fingers[i];
    const FingerState& newer = second.fingers[i];
    FingerState* mid = &out->fingers[i];
    mid->touch_major = (older.touch_major + newer.touch_major) / 2.0;
    mid->touch_minor = (older.touch_minor + newer.touch_minor) / 2.0;
    mid->width_major = (older.width_major + newer.width_major) / 2.0;
    mid->width_minor = (older.width_minor + newer.width_minor) / 2.0;
    mid->pressure = (older.pressure + newer.pressure) / 2.0;
    mid->orientation = (older.orientation + newer.orientation) / 2.0;
    mid->position_x = (older.position_x + newer.position_x) / 2.0;
    mid->position_y = (older.position_y + newer.position_y) / 2.0;
    mid->tracking_id = older.tracking_id;
    mid->flags = newer.flags;
  }
  // We are not interested in interpolating relative movement values.
  out->rel_x = 0;
  out->rel_y = 0;
  out->rel_wheel = 0;
  out->rel_hwheel = 0;
}

void LookaheadFilterInterpreter::AssignTrackingIds() {
  // For semi-mt devices, drumrolls and quick moves are handled in
  // SemiMtCorrectingFilterInterpreter already. We need to bypass the detection
  // and tracking id reassignment here to make fast-scroll working correctly.
  if (hwprops_->support_semi_mt || !drumroll_suppression_enable_.val_)
    return;
  if (queue_.size() < 2) {
    // Always reassign trackingID on the very first hwstate so that
    // the next hwstate can inherit the trackingID mapping.
    if (queue_.size() == 1) {
      QState* tail = queue_.Tail();
      HardwareState* hs = &tail->state_;
      for (size_t i = 0; i < hs->finger_cnt; i++) {
        FingerState* fs = &hs->fingers[i];
        tail->output_ids_[fs->tracking_id] = NextTrackingId();
        fs->tracking_id = tail->output_ids_[fs->tracking_id];
      }
      if (hs->finger_cnt > 0)
        tail->due_ += ExtraVariableDelay();
    }
    return;
  }

  QState* tail = queue_.Tail();
  HardwareState* hs = &tail->state_;
  QState* prev_qs = queue_.size() < 2 ? NULL : tail->prev_;
  HardwareState* prev_hs = prev_qs ? &prev_qs->state_ : NULL;
  QState* prev2_qs = queue_.size() < 3 ? NULL : prev_qs->prev_;
  HardwareState* prev2_hs = prev2_qs ? &prev2_qs->state_ : NULL;

  RemoveMissingIdsFromMap(&tail->output_ids_, *hs);
  float dt = prev_hs ? hs->timestamp - prev_hs->timestamp : 1.0;
  float prev_dt =
      prev_hs && prev2_hs ? prev_hs->timestamp - prev2_hs->timestamp : 1.0;

  float dist_sq_thresh =
      dt * dt * drumroll_speed_thresh_.val_ * drumroll_speed_thresh_.val_;

  const float multiplier_per_time_ratio_sq = dt * dt *
      drumroll_max_speed_ratio_.val_ *
      drumroll_max_speed_ratio_.val_;
  const float prev_dt_sq = prev_dt * prev_dt;

  set<short, kMaxFingers> separated_fingers;  // input ids
  float max_dist_sq = 0.0;  // largest non-drumroll dist squared.
  // If there is only a single finger drumrolling, this is the distance
  // it travelled squared.
  float drum_dist_sq = INFINITY;
  bool new_finger_present = false;
  for (size_t i = 0; i < hs->finger_cnt; i++) {
    FingerState* fs = &hs->fingers[i];
    const short old_id = fs->tracking_id;
    bool new_finger = false;
    if (!MapContainsKey(tail->output_ids_, fs->tracking_id)) {
      tail->output_ids_[fs->tracking_id] = NextTrackingId();
      new_finger_present = new_finger = true;
    }
    fs->tracking_id = tail->output_ids_[fs->tracking_id];
    if (new_finger)
      continue;
    if (!prev_hs) {
      Err("How is prev_hs NULL?");
      continue;
    }
    // Consider breaking the connection between this frame and the previous
    // by assigning this finger a new ID
    if (!MapContainsKey(prev_qs->output_ids_, old_id)) {
      Err("How is old id missing from old output_ids?");
      continue;
    }
    FingerState* prev_fs =
        prev_hs->GetFingerState(prev_qs->output_ids_[old_id]);
    if (!prev_fs) {
      Err("How is prev_fs NULL?");
      continue;
    }

    float dx = fs->position_x - prev_fs->position_x;
    float dy = fs->position_y - prev_fs->position_y;
    float dist_sq = dx * dx + dy * dy;
    float prev_max_dist_sq = max_dist_sq;
    if (dist_sq > max_dist_sq)
      max_dist_sq = dist_sq;

    FingerState* prev2_fs = NULL;

    if (prev2_hs && MapContainsKey(prev2_qs->output_ids_, old_id))
      prev2_fs = prev2_hs->GetFingerState(prev2_qs->output_ids_[old_id]);

    // Quick movement detection.
    if (prev2_fs) {
      float prev_dx = prev_fs->position_x - prev2_fs->position_x;
      float prev_dy = prev_fs->position_y - prev2_fs->position_y;

      // Along either x or y axis, the movement between (prev2, prev) and
      // (prev, current) should be on the same direction, and the distance
      // travelled should be larger than quick_move_thresh_.
      if ((prev_dx * dx >= 0.0 && fabs(prev_dx) >= quick_move_thresh_.val_ &&
           fabs(dx) >= quick_move_thresh_.val_) ||
          (prev_dy * dy >= 0.0 && fabs(prev_dy) >= quick_move_thresh_.val_ &&
           fabs(dy) >= quick_move_thresh_.val_)) {
        // Quick movement detected. Correct the tracking ID if the previous
        // finger state has a reassigned trackingID due to drumroll detection.
        if (prev_qs->output_ids_[old_id] != prev2_qs->output_ids_[old_id]) {
          prev_qs->output_ids_[old_id] = prev2_qs->output_ids_[old_id];
          prev_fs->tracking_id = prev_qs->output_ids_[old_id];
          tail->output_ids_[old_id] = prev2_qs->output_ids_[old_id];
          fs->tracking_id = tail->output_ids_[old_id];
          continue;
        }
      }
    }

    // Drumroll detection.
    if (dist_sq > dist_sq_thresh) {
      if (prev2_fs) {
        float prev_dx = prev_fs->position_x - prev2_fs->position_x;
        float prev_dy = prev_fs->position_y - prev2_fs->position_y;
        // If the finger is switching direction rapidly, it's drumroll.
        if (prev_dx * dx >= 0.0 || prev_dy * dy >= 0.0) {
          // Finger not switching direction rapidly. Now, test if large
          // speed change.
          float prev_dist_sq = prev_dx * prev_dx + prev_dy * prev_dy;
          if (dist_sq * prev_dt_sq <=
              multiplier_per_time_ratio_sq * prev_dist_sq)
            continue;
        }
      }
      if (fs->flags & (GESTURES_FINGER_WARP_X | GESTURES_FINGER_WARP_Y)) {
        // Finger is warping. Don't reassign tracking ID.
        // Because we would have reassigned the ID, we make sure we're warping
        // both axes
        fs->flags |= (GESTURES_FINGER_WARP_X | GESTURES_FINGER_WARP_Y);
        continue;
      }
      separated_fingers.insert(old_id);
      SeparateFinger(tail, fs, old_id);
      // Separating fingers shouldn't tap.
      fs->flags |= GESTURES_FINGER_NO_TAP;
      // Try to also flag the previous frame, if we didn't execute it yet
      if (!prev_qs->completed_)
        prev_fs->flags |= GESTURES_FINGER_NO_TAP;
      // Since this is drumroll, don't count it toward the max dist sq.
      max_dist_sq = prev_max_dist_sq;
      // Instead, store this distance as the drumroll distance
      drum_dist_sq = dist_sq;
    }
  }
  // There are some times where we abort drumrolls. If two fingers are both
  // drumrolling, that's unlikely (they are probably quickly swiping). Also,
  // if a single finger is moving enough to trigger drumroll, but another
  // finger is moving about as much, don't drumroll-suppress the one finger.
  if (separated_fingers.size() > 1 ||
      (separated_fingers.size() == 1 &&
       drum_dist_sq <
       max_dist_sq * co_move_ratio_.val_ * co_move_ratio_.val_)) {
    // Two fingers drumrolling at the exact same time. More likely this is
    // a fast multi-finger swipe. Abort the drumroll detection.
    for (set<short, kMaxFingers>::const_iterator it = separated_fingers.begin(),
             e = separated_fingers.end(); it != e; ++it) {
      short input_id = *it;
      if (!MapContainsKey(prev_qs->output_ids_, input_id)) {
        Err("How is input ID missing from prev state? %d", input_id);
        continue;
      }
      short new_bad_output_id = tail->output_ids_[input_id];
      short prev_output_id = prev_qs->output_ids_[input_id];
      tail->output_ids_[input_id] = prev_output_id;
      FingerState* fs = tail->state_.GetFingerState(new_bad_output_id);
      if (!fs) {
        Err("Can't find finger state.");
        continue;
      }
      fs->tracking_id = prev_output_id;
    }
    separated_fingers.clear();
  }

  if (!separated_fingers.empty() || new_finger_present ||
      (delay_on_possible_liftoff_.val_ && hs && prev_hs && prev2_hs &&
       LiftoffJumpStarting(*hs, *prev_hs, *prev2_hs))) {
    // Possibly add some extra delay to correct, incase this separation
    // shouldn't have occurred or if the finger may be lifting from the pad.
    tail->due_ += ExtraVariableDelay();
  }
}

bool LookaheadFilterInterpreter::LiftoffJumpStarting(
    const HardwareState& hs,
    const HardwareState& prev_hs,
    const HardwareState& prev2_hs) const {
  for (size_t i = 0; i < hs.finger_cnt; i++) {
    const FingerState* fs = &hs.fingers[i];
    const FingerState* prev_fs = prev_hs.GetFingerState(fs->tracking_id);
    if (!prev_fs)
      continue;
    if (fs->pressure > prev_fs->pressure) {
      // Pressure increasing. Likely not liftoff.
      continue;
    }
    const FingerState* prev2_fs = prev2_hs.GetFingerState(fs->tracking_id);
    if (!prev2_fs)
      continue;

    float dist_sq_new = DistSq(*fs, *prev_fs);
    float dist_sq_old = DistSq(*prev_fs, *prev2_fs);
    float dt_new = hs.timestamp - prev_hs.timestamp;
    float dt_old = prev_hs.timestamp - prev2_hs.timestamp;

    if (dt_old * dt_old * dist_sq_new >
        dt_new * dt_new * dist_sq_old *
        liftoff_speed_increase_threshold_.val_ *
        liftoff_speed_increase_threshold_.val_)
      return true;
  }
  return false;
}

void LookaheadFilterInterpreter::TapDownOccurringGesture(stime_t now) {
  if (suppress_immediate_tapdown_.val_)
    return;
  if (queue_.size() < 2)
    return;  // Not enough data to know
  HardwareState& hs = queue_.Tail()->state_;
  if (queue_.Tail()->state_.timestamp != now)
    return;  // We didn't push a new hardware state now
  // See if latest hwstate has finger that previous doesn't
  HardwareState& prev_hs = queue_.Tail()->prev_->state_;
  if (hs.finger_cnt > prev_hs.finger_cnt) {
    // Finger was added.
    ProduceGesture(Gesture(kGestureFling, prev_hs.timestamp, hs.timestamp,
                           0, 0, GESTURES_FLING_TAP_DOWN));
    return;
  }
  // Go finger by finger for a final check
  for (size_t i = 0; i < hs.finger_cnt; i++)
    if (!prev_hs.GetFingerState(hs.fingers[i].tracking_id)) {
      ProduceGesture(Gesture(kGestureFling, prev_hs.timestamp, hs.timestamp,
                             0, 0, GESTURES_FLING_TAP_DOWN));
      return;
    }
}

void LookaheadFilterInterpreter::SeparateFinger(QState* node,
                                                FingerState* fs,
                                                short input_id) {
  short output_id = NextTrackingId();
  if (!MapContainsKey(node->output_ids_, input_id)) {
    Err("How is this possible?");
    return;
  }
  node->output_ids_[input_id] = output_id;
  fs->tracking_id = output_id;
}

short LookaheadFilterInterpreter::NextTrackingId() {
  short out = ++last_id_ & 0x7fff;  // keep it non-negative
  return out;
}

void LookaheadFilterInterpreter::AttemptInterpolation() {
  if (queue_.size() < 2)
    return;
  QState* new_node = queue_.Tail();
  QState* prev = new_node->prev_;
  if (new_node->state_.timestamp - prev->state_.timestamp <
      split_min_period_.val_)
    return;  // Nodes came in too quickly to need interpolation
  if (!prev->state_.SameFingersAs(new_node->state_))
    return;
  QState* node = free_list_.PopFront();
  if (!node) {
    Err("out of nodes?");
    return;
  }
  node->state_.fingers = node->fs_.get();
  node->completed_ = false;
  Interpolate(prev->state_, new_node->state_, &node->state_);

  double delay = max(0.0, min<stime_t>(kMaxDelay, min_delay_.val_));
  node->due_ = node->state_.timestamp + delay;

  if (node->state_.timestamp <= last_interpreted_time_) {
    // Time wouldn't seem monotonically increasing w/ this new event, so
    // discard it.
    free_list_.PushBack(node);
    return;
  }

  queue_.InsertBefore(new_node, node);
}

void LookaheadFilterInterpreter::HandleTimerImpl(stime_t now,
                                                 stime_t* timeout) {
  TapDownOccurringGesture(now);
  stime_t next_timeout = -1.0;

  while (true) {
    if (interpreter_due_ > 0.0) {
      if (interpreter_due_ > now) {
        next_timeout = interpreter_due_ - now;
        break;  // Spurious callback
      }
      next_timeout = -1.0;
      last_interpreted_time_ = now;
      next_->HandleTimer(now, &next_timeout);
    } else {
      if (queue_.Empty())
        break;
      // Get next uncompleted and overdue hwstate
      QState* node = queue_.Head();
      while (node != queue_.Tail() && node->completed_)
        node = node->next_;
      if (node->completed_ || node->due_ > now)
        break;
      next_timeout = -1.0;
      last_interpreted_time_ = node->state_.timestamp;
      const size_t finger_cnt = node->state_.finger_cnt;
      FingerState fs_copy[finger_cnt];
      std::copy(&node->state_.fingers[0],
                &node->state_.fingers[finger_cnt],
                &fs_copy[0]);
      HardwareState hs_copy = {
        node->state_.timestamp,
        node->state_.buttons_down,
        node->state_.finger_cnt,
        node->state_.touch_cnt,
        fs_copy,
        node->state_.rel_x,
        node->state_.rel_y,
        node->state_.rel_wheel,
        node->state_.rel_hwheel,
      };
      next_->SyncInterpret(&hs_copy, &next_timeout);

      // Clear previously completed nodes, but keep at least two nodes.
      while (queue_.size() > 2 && queue_.Head()->completed_)
        free_list_.PushBack(queue_.PopFront());

      // Mark current node completed. This should be the only completed
      // node in the queue.
      node->completed_ = true;
    }
    UpdateInterpreterDue(next_timeout, now, timeout);
  }
  UpdateInterpreterDue(next_timeout, now, timeout);
}

void LookaheadFilterInterpreter::ConsumeGesture(const Gesture& gesture) {
  QState* node = queue_.Head();

  float distance_sq = 0.0;
  // Slow movements should potentially be suppressed
  switch (gesture.type) {
    case kGestureTypeMove:
      distance_sq = gesture.details.move.dx * gesture.details.move.dx +
          gesture.details.move.dy * gesture.details.move.dy;
      break;
    case kGestureTypeScroll:
      distance_sq = gesture.details.scroll.dx * gesture.details.scroll.dx +
          gesture.details.scroll.dy * gesture.details.scroll.dy;
      break;
    default:
      // Non-movement: just allow it.
      ProduceGesture(gesture);
      return;
  }
  stime_t time_delta = gesture.end_time - gesture.start_time;
  float min_nonsuppress_dist_sq =
      min_nonsuppress_speed_.val_ * min_nonsuppress_speed_.val_ *
      time_delta * time_delta;
  if (distance_sq >= min_nonsuppress_dist_sq) {
    ProduceGesture(gesture);
    return;
  }
  // Speed is slow. Suppress if fingers have changed.
  for (QState* iter = node->next_; iter != queue_.End(); iter = iter->next_)
    if (!node->state_.SameFingersAs(iter->state_) ||
        (node->state_.buttons_down != iter->state_.buttons_down))
      return; // suppress

  ProduceGesture(gesture);
}

void LookaheadFilterInterpreter::UpdateInterpreterDue(
    stime_t new_interpreter_timeout,
    stime_t now,
    stime_t* timeout) {
  // The next hardware state may already be over due, thus having a negative
  // timeout, so we use -DBL_MAX as the invalid value.
  stime_t next_hwstate_timeout = -DBL_MAX;
  // Scan queue_ to find when next hwstate is due.
  for (QState* node = queue_.Begin(); node != queue_.End();
       node = node->next_) {
    if (node->completed_)
      continue;
    next_hwstate_timeout = node->due_ - now;
    break;
  }

  interpreter_due_ = -1.0;
  if (new_interpreter_timeout >= 0.0 &&
      (new_interpreter_timeout < next_hwstate_timeout ||
       next_hwstate_timeout == -DBL_MAX)) {
    interpreter_due_ = new_interpreter_timeout + now;
    *timeout = new_interpreter_timeout;
  } else if (next_hwstate_timeout > -DBL_MAX) {
    *timeout = next_hwstate_timeout;
  }
}

void LookaheadFilterInterpreter::Initialize(
    const HardwareProperties* hwprops,
    Metrics* metrics,
    MetricsProperties* mprops,
    GestureConsumer* consumer) {
  FilterInterpreter::Initialize(hwprops, NULL, mprops, consumer);
  const size_t kMaxQNodes = 16;
  queue_.DeleteAll();
  free_list_.DeleteAll();
  for (size_t i = 0; i < kMaxQNodes; ++i) {
    QState* node = new QState(hwprops_->max_finger_cnt);
    free_list_.PushBack(node);
  }
}

stime_t LookaheadFilterInterpreter::ExtraVariableDelay() const {
  return std::max<stime_t>(0.0, max_delay_.val_ - min_delay_.val_);
}

LookaheadFilterInterpreter::QState::QState()
    : max_fingers_(0), completed_(false), next_(NULL), prev_(NULL) {
  fs_.reset();
  state_.fingers = NULL;
}

LookaheadFilterInterpreter::QState::QState(unsigned short max_fingers)
    : max_fingers_(max_fingers), completed_(false), next_(NULL), prev_(NULL) {
  fs_.reset(new FingerState[max_fingers]);
  state_.fingers = fs_.get();
}

void LookaheadFilterInterpreter::QState::set_state(
    const HardwareState& new_state) {
  state_.timestamp = new_state.timestamp;
  state_.buttons_down = new_state.buttons_down;
  state_.touch_cnt = new_state.touch_cnt;
  unsigned short copy_count = new_state.finger_cnt;
  if (new_state.finger_cnt > max_fingers_) {
    Err("State with too many fingers! (%u vs %u)",
        new_state.finger_cnt,
        max_fingers_);
    copy_count = max_fingers_;
  }
  state_.finger_cnt = copy_count;
  std::copy(new_state.fingers, new_state.fingers + copy_count, state_.fingers);
  state_.rel_x = new_state.rel_x;
  state_.rel_y = new_state.rel_y;
  state_.rel_wheel = new_state.rel_wheel;
  state_.rel_hwheel = new_state.rel_hwheel;
}

}  // namespace gestures
