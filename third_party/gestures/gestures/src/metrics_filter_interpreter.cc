// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/metrics_filter_interpreter.h"

#include <cmath>

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/logging.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"
#include "gestures/include/util.h"

namespace gestures {

MetricsFilterInterpreter::MetricsFilterInterpreter(
    PropRegistry* prop_reg,
    Interpreter* next,
    Tracer* tracer,
    GestureInterpreterDeviceClass devclass)
    : FilterInterpreter(NULL, next, tracer, false),
      mstate_mm_(kMaxFingers * MState::MaxHistorySize()),
      history_mm_(kMaxFingers),
      devclass_(devclass),
      mouse_movement_session_index_(0),
      mouse_movement_current_session_length(0),
      mouse_movement_current_session_start(0),
      mouse_movement_current_session_last(0),
      mouse_movement_current_session_distance(0),
      noisy_ground_distance_threshold_(prop_reg,
                                       "Metrics Noisy Ground Distance",
                                       10.0),
      noisy_ground_time_threshold_(prop_reg, "Metrics Noisy Ground Time", 0.1),
      mouse_moving_time_threshold_(prop_reg,
                                   "Metrics Mouse Moving Time",
                                   0.05),
      mouse_control_warmup_sessions_(prop_reg,
                                     "Metrics Mouse Warmup Session",
                                     100) {
  InitName();
}

void MetricsFilterInterpreter::SyncInterpretImpl(HardwareState* hwstate,
                                                   stime_t* timeout) {
  if (devclass_ == GESTURES_DEVCLASS_TOUCHPAD) {
    // Right now, we only want to update finger states for built-in touchpads
    // because all the generated metrics gestures would be put under each
    // platform's hat on the Chrome UMA dashboard. If we send metrics gestures
    // (e.g. noisy ground instances) for external peripherals (e.g. multi-touch
    // mice), they would be mistaken as from the platform's touchpad and thus
    // results in over-counting.
    //
    // TODO(sheckylin): Don't send metric gestures for external touchpads
    // either.
    // TODO(sheckylin): Track finger related metrics for external peripherals
    // as well after gaining access to the UMA log.
    UpdateFingerState(*hwstate);
  } else if (devclass_ == GESTURES_DEVCLASS_MOUSE ||
             devclass_ == GESTURES_DEVCLASS_MULTITOUCH_MOUSE) {
    UpdateMouseMovementState(*hwstate);
  }
  next_->SyncInterpret(hwstate, timeout);
}

template <class StateType, class DataType>
void MetricsFilterInterpreter::AddNewStateToBuffer(
    MemoryManagedList<StateType>* history,
    const DataType& data,
    const HardwareState& hwstate) {
  // The history buffer is already full, pop one
  if (history->size() == StateType::MaxHistorySize())
    history->DeleteFront();

  // Push the new finger state to the back of buffer
  StateType* current = history->PushNewEltBack();
  if (!current) {
    Err("MetricsFilterInterpreter buffer out of space");
    return;
  }
  current->Init(data, hwstate);
}

void MetricsFilterInterpreter::UpdateMouseMovementState(
    const HardwareState& hwstate) {
  // Skip finger-only hardware states for multi-touch mice.
  if (hwstate.rel_x == 0 && hwstate.rel_y == 0)
    return;

  // If the last movement is too long ago, we consider the history
  // an independent session. Report statistic for it and start a new
  // one.
  if (mouse_movement_current_session_length >= 1 &&
      (hwstate.timestamp - mouse_movement_current_session_last >
       mouse_moving_time_threshold_.val_)) {
    // We skip the first a few sessions right after the user starts using the
    // mouse because they tend to be more noisy.
    if (mouse_movement_session_index_ >= mouse_control_warmup_sessions_.val_)
      ReportMouseStatistics();
    mouse_movement_current_session_length = 0;
    mouse_movement_current_session_distance = 0;
    ++mouse_movement_session_index_;
  }

  // We skip the movement of the first event because there is no way to tell
  // the start time of it.
  if (!mouse_movement_current_session_length) {
    mouse_movement_current_session_start = hwstate.timestamp;
  } else {
    mouse_movement_current_session_distance +=
        sqrtf(hwstate.rel_x * hwstate.rel_x + hwstate.rel_y * hwstate.rel_y);
  }
  mouse_movement_current_session_last = hwstate.timestamp;
  ++mouse_movement_current_session_length;
}

void MetricsFilterInterpreter::ReportMouseStatistics() {
  // At least 2 samples are needed to compute delta t.
  if (mouse_movement_current_session_length == 1)
    return;

  // Compute the average speed.
  stime_t session_time = mouse_movement_current_session_last -
                         mouse_movement_current_session_start;
  double avg_speed = mouse_movement_current_session_distance / session_time;

  // Send the metrics gesture.
  ProduceGesture(Gesture(kGestureMetrics,
                         mouse_movement_current_session_start,
                         mouse_movement_current_session_last,
                         kGestureMetricsTypeMouseMovement,
                         avg_speed,
                         session_time));
}

void MetricsFilterInterpreter::UpdateFingerState(
    const HardwareState& hwstate) {
  FingerHistoryMap removed;
  RemoveMissingIdsFromMap(&histories_, hwstate, &removed);
  for (FingerHistoryMap::const_iterator it =
       removed.begin(); it != removed.end(); ++it) {
    it->second->DeleteAll();
    history_mm_.Free(it->second);
}

  FingerState *fs = hwstate.fingers;
  for (short i = 0; i < hwstate.finger_cnt; i++) {
    FingerHistory* hp;

    // Update the map if the contact is new
    if (!MapContainsKey(histories_, fs[i].tracking_id)) {
      hp = history_mm_.Allocate();
      if (!hp) {
        Err("FingerHistory out of space");
        continue;
      }
      hp->Init(&mstate_mm_);
      histories_[fs[i].tracking_id] = hp;
    } else {
      hp = histories_[fs[i].tracking_id];
    }

    // Check if the finger history contains interesting patterns
    AddNewStateToBuffer(hp, fs[i], hwstate);
    DetectNoisyGround(hp);
  }
}

bool MetricsFilterInterpreter::DetectNoisyGround(
    const FingerHistory* history) {
  MState* current = history->Tail();
  size_t n_samples = history->size();
  // Noise pattern takes 3 samples
  if (n_samples < 3)
    return false;

  MState* past_1 = current->prev_;
  MState* past_2 = past_1->prev_;
  // Noise pattern needs to happen in a short period of time
  if(current->timestamp - past_2->timestamp >
      noisy_ground_time_threshold_.val_) {
    return false;
  }

  // vec[when][x,y]
  float vec[2][2];
  vec[0][0] = current->data.position_x - past_1->data.position_x;
  vec[0][1] = current->data.position_y - past_1->data.position_y;
  vec[1][0] = past_1->data.position_x - past_2->data.position_x;
  vec[1][1] = past_1->data.position_y - past_2->data.position_y;
  const float thr = noisy_ground_distance_threshold_.val_;
  // We dictate the noise pattern as two consecutive big moves in
  // opposite directions in either X or Y
  for (size_t i = 0; i < arraysize(vec[0]); i++)
    if ((vec[0][i] < -thr && vec[1][i] > thr) ||
        (vec[0][i] > thr && vec[1][i] < -thr)) {
      ProduceGesture(Gesture(kGestureMetrics, past_2->timestamp,
                     current->timestamp, kGestureMetricsTypeNoisyGround,
                     vec[0][i], vec[1][i]));
      return true;
    }
  return false;
}

}  // namespace gestures
