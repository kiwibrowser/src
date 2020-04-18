// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/fling_stop_filter_interpreter.h"

#include "gestures/include/util.h"

namespace gestures {

FlingStopFilterInterpreter::FlingStopFilterInterpreter(PropRegistry* prop_reg,
                                                       Interpreter* next,
                                                       Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      already_extended_(false),
      prev_touch_cnt_(0),
      fling_stop_deadline_(0.0),
      next_timer_deadline_(0.0),
      fling_stop_timeout_(prop_reg, "Fling Stop Timeout", 0.03),
      fling_stop_extra_delay_(prop_reg, "Fling Stop Extra Delay", 0.055) {
  InitName();
}

void FlingStopFilterInterpreter::SyncInterpretImpl(HardwareState* hwstate,
                                                   stime_t* timeout) {
  fingers_of_last_hwstate_.clear();
  for (int i = 0; i < hwstate->finger_cnt; i++)
    fingers_of_last_hwstate_.insert(hwstate->fingers[i].tracking_id);

  UpdateFlingStopDeadline(*hwstate);

  stime_t next_timeout = -1.0;
  if (fling_stop_deadline_ != 0.0) {
    if (!already_extended_ && NeedsExtraTime(*hwstate)) {
      fling_stop_deadline_ += fling_stop_extra_delay_.val_;
      already_extended_ = true;
    }
    if (hwstate->timestamp > fling_stop_deadline_) {
      // sub in a fling before processing other interpreters
      ProduceGesture(Gesture(kGestureFling, prev_timestamp_,
                             hwstate->timestamp, 0.0, 0.0,
                             GESTURES_FLING_TAP_DOWN));
      fling_stop_deadline_ = 0.0;
    }
  }
  next_->SyncInterpret(hwstate, &next_timeout);
  *timeout = SetNextDeadlineAndReturnTimeoutVal(hwstate->timestamp,
                                                next_timeout);
}

bool FlingStopFilterInterpreter::NeedsExtraTime(
    const HardwareState& hwstate) const {
  int num_new_fingers = 0;
  for (int i = 0; i < hwstate.finger_cnt; i++) {
    const short id = hwstate.fingers[i].tracking_id;
    if (!SetContainsValue(fingers_present_for_last_fling_, id)) {
      num_new_fingers++;
    }
  }

  return (num_new_fingers >= 2);
}

void FlingStopFilterInterpreter::ConsumeGesture(const Gesture& gesture) {
  if (gesture.type == kGestureTypeFling) {
    fingers_present_for_last_fling_ = fingers_of_last_hwstate_;
    already_extended_ = false;
  }

  if (fling_stop_deadline_ != 0.0) {
    if (gesture.type == kGestureTypeScroll) {
      // Don't need to stop fling, since the user is scrolling
      Gesture copy = gesture;
      copy.details.scroll.stop_fling = 1;
      fling_stop_deadline_ = 0.0;
      ProduceGesture(copy);
      return;
    } else if (gesture.type == kGestureTypeButtonsChange) {
      // sub in a fling before the button
      ProduceGesture(Gesture(kGestureFling, gesture.start_time,
                             gesture.start_time, 0.0, 0.0,
                             GESTURES_FLING_TAP_DOWN));
      fling_stop_deadline_ = 0.0;
    }
  }
  ProduceGesture(gesture);
}

void FlingStopFilterInterpreter::UpdateFlingStopDeadline(
    const HardwareState& hwstate) {
  if (fling_stop_timeout_.val_ <= 0.0)
    return;

  stime_t now = hwstate.timestamp;
  bool finger_added = hwstate.touch_cnt > prev_touch_cnt_;

  if (finger_added && fling_stop_deadline_ == 0.0) {
    // first finger added in a while. Note it.
    fling_stop_deadline_ = now + fling_stop_timeout_.val_;
    return;
  }

  prev_timestamp_ = now;
  prev_touch_cnt_ = hwstate.touch_cnt;
}

stime_t FlingStopFilterInterpreter::SetNextDeadlineAndReturnTimeoutVal(
    stime_t now,
    stime_t next_timeout) {
  next_timer_deadline_ = next_timeout >= 0.0 ? now + next_timeout : 0.0;
  stime_t local_timeout = fling_stop_deadline_ == 0.0 ? -1.0 :
      std::max(fling_stop_deadline_ - now, 0.0);

  if (next_timeout < 0.0 && local_timeout < 0.0)
    return -1.0;
  if (next_timeout < 0.0)
    return local_timeout;
  if (local_timeout < 0.0)
    return next_timeout;
  return std::min(next_timeout, local_timeout);
}

void FlingStopFilterInterpreter::HandleTimerImpl(stime_t now,
                                                     stime_t* timeout) {
  bool call_next = false;
  if (fling_stop_deadline_ > 0.0 && next_timer_deadline_ > 0.0)
    call_next = fling_stop_deadline_ > next_timer_deadline_;
  else
    call_next = next_timer_deadline_ > 0.0;

  if (!call_next) {
    if (fling_stop_deadline_ > now) {
      Err("Spurious callback. now: %f, fs deadline: %f, next deadline: %f",
          now, fling_stop_deadline_, next_timer_deadline_);
      return;
    }
    fling_stop_deadline_ = 0.0;
    ProduceGesture(Gesture(kGestureFling, prev_timestamp_,
                           now, 0.0, 0.0,
                           GESTURES_FLING_TAP_DOWN));
    stime_t next_timeout = next_timer_deadline_ == 0.0 ? -1.0 :
        std::max(0.0, next_timer_deadline_ - now);
    *timeout = SetNextDeadlineAndReturnTimeoutVal(now, next_timeout);
    return;
  }
  // Call next_
  if (next_timer_deadline_ > now) {
    Err("Spurious callback. now: %f, fs deadline: %f, next deadline: %f",
        now, fling_stop_deadline_, next_timer_deadline_);
    return;
  }
  stime_t next_timeout = -1.0;
  next_->HandleTimer(now, &next_timeout);
  *timeout = SetNextDeadlineAndReturnTimeoutVal(now, next_timeout);
}

}  // namespace gestures
