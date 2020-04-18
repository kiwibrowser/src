// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>  // for FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/list.h"
#include "gestures/include/map.h"
#include "gestures/include/memory_manager.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_METRICS_FILTER_INTERPRETER_H_
#define GESTURES_METRICS_FILTER_INTERPRETER_H_

namespace gestures {

// This interpreter catches scenarios which we want to collect UMA stats for
// and generate GestureMetrics gestures for them. Those gestures would picked
// up in Chrome to log the desired UMA stats. We chose not to call out to the
// metrics lib here because it might introduce the deadlock problem.

class MetricsFilterInterpreter : public FilterInterpreter {
 public:
  // Takes ownership of |next|:
  MetricsFilterInterpreter(PropRegistry* prop_reg,
                           Interpreter* next,
                           Tracer* tracer,
                           GestureInterpreterDeviceClass devclass);
  virtual ~MetricsFilterInterpreter() {}

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

 private:
  template <class DataType, size_t kHistorySize>
  struct State {
    State() {}
    State(const DataType& fs, const HardwareState& hwstate) {
      Init(fs, hwstate);
    }

    void Init(const DataType& fs, const HardwareState& hwstate) {
      timestamp = hwstate.timestamp;
      data = fs;
    }

    static size_t MaxHistorySize() { return kHistorySize; }

    stime_t timestamp;
    DataType data;
    State<DataType, kHistorySize>* next_;
    State<DataType, kHistorySize>* prev_;
  };

  // struct for one finger's data of one frame.
  typedef State<FingerState, 3> MState;
  typedef MemoryManagedList<MState> FingerHistory;

  // Push the new data into the buffer.
  template <class StateType, class DataType>
  void AddNewStateToBuffer(MemoryManagedList<StateType>* history,
                           const DataType& data,
                           const HardwareState& hwstate);

  // Update the class with new finger data, check if there is any interesting
  // pattern
  void UpdateFingerState(const HardwareState& hwstate);

  // Detect the noisy ground pattern and send GestureMetrics
  bool DetectNoisyGround(const FingerHistory* history);

  // Update the class with new mouse movement data.
  void UpdateMouseMovementState(const HardwareState& hwstate);

  // Compute interested statistics for the mouse history, send GestureMetrics.
  void ReportMouseStatistics();

  // memory managers to prevent malloc during interrupt calls
  MemoryManager<MState> mstate_mm_;
  MemoryManager<FingerHistory> history_mm_;

  // A map to store each finger's past data
  typedef map<short, FingerHistory*, kMaxFingers> FingerHistoryMap;
  FingerHistoryMap histories_;

  // Device class (e.g. touchpad, mouse).
  GestureInterpreterDeviceClass devclass_;

  // The total number of mouse movement sessions from the startup.
  int mouse_movement_session_index_;

  // The number of events in the current mouse movement session.
  int mouse_movement_current_session_length;

  // The start/last update time of the current mouse movement session.
  stime_t mouse_movement_current_session_start;
  stime_t mouse_movement_current_session_last;

  // The total distance that mouse traveled in the current mouse movement
  // session.
  double mouse_movement_current_session_distance;

  // Threshold values for detecting movements caused by the noisy ground
  DoubleProperty noisy_ground_distance_threshold_;
  DoubleProperty noisy_ground_time_threshold_;

  // Threshold values for determining a moving mouse. We consider the mouse
  // stationary if it doesn't report movements in the last threshold amount
  // of time.
  DoubleProperty mouse_moving_time_threshold_;

  // Number of mouse movement sessions that we skip at startup. We do this
  // because it takes time for the user to "get used to" the mouse speed when
  // they first start using the mouse. We only want to capture the user metrics
  // after the user has been familiar with their mouse.
  IntProperty mouse_control_warmup_sessions_;
};

}  // namespace gestures

#endif  // GESTURES_METRICS_FILTER_INTERPRETER_H_
