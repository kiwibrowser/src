// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include <gtest/gtest.h>

#include "gestures/include/activity_log.h"
#include "gestures/include/gestures.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_INTERPRETER_H__
#define GESTURES_INTERPRETER_H__

// This is a collection of supporting structs and an interface for
// Interpreters.

struct HardwareState;

namespace gestures {

class GestureConsumer {
 public:
  virtual ~GestureConsumer() {}
  virtual void ConsumeGesture(const Gesture& gesture) = 0;
};

class Metrics;
class MetricsProperties;

// Interface for all interpreters. Interpreters currently are synchronous.
// A synchronous interpreter will return  0 or 1 Gestures for each passed in
// HardwareState.
class Interpreter {
  FRIEND_TEST(InterpreterTest, ResetLogTest);
  FRIEND_TEST(LoggingFilterInterpreterTest, LogResetHandlerTest);
 public:
  Interpreter(PropRegistry* prop_reg, Tracer* tracer, bool force_logging);
  virtual ~Interpreter();

  // Called to interpret the current state.
  // The passed |hwstate| may be modified.
  // If *timeout is set to >0.0, a timer will be setup to call
  // HandleTimer after *timeout time passes. An interpreter can only
  // have up to 1 outstanding timer, so if a timeout is requested by
  // setting *timeout and one already exists, the old one will be cancelled
  // and reused for this timeout.
  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout);

  // Called to handle a timeout.
  // If *timeout is set to >0.0, a timer will be setup to call
  // HandleTimer after *timeout time passes. An interpreter can only
  // have up to 1 outstanding timer, so if a timeout is requested by
  // setting *timeout and one already exists, the old one will be cancelled
  // and reused for this timeout.
  virtual void HandleTimer(stime_t now, stime_t* timeout);

  virtual void Initialize(const HardwareProperties* hwprops,
                          Metrics* metrics, MetricsProperties* mprops,
                          GestureConsumer* consumer);

  virtual Json::Value EncodeCommonInfo();
  std::string Encode();

  virtual void Clear() {
    if (log_.get())
      log_->Clear();
  }

  virtual void ProduceGesture(const Gesture& gesture);
  const char* name() const { return name_; }

 protected:
  std::unique_ptr<ActivityLog> log_;
  GestureConsumer* consumer_;
  const HardwareProperties* hwprops_;
  Metrics* metrics_;
  std::unique_ptr<Metrics> own_metrics_;
  bool requires_metrics_;
  bool initialized_;

  void InitName();
  void Trace(const char* message, const char* name);

  virtual void SyncInterpretImpl(HardwareState* hwstate,
                                 stime_t* timeout) {}
  virtual void HandleTimerImpl(stime_t now, stime_t* timeout) {}

 private:
  const char* name_;
  Tracer* tracer_;
  void LogOutputs(const Gesture* result, stime_t* timeout, const char* action);
};
}  // namespace gestures

#endif  // GESTURES_INTERPRETER_H__
