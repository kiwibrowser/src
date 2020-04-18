// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include <json/value.h>

#include "gestures/include/interpreter.h"
#include "gestures/include/macros.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_FILTER_INTERPRETER_H__
#define GESTURES_FILTER_INTERPRETER_H__

namespace gestures {

// Interface for all filter interpreters.

class FilterInterpreter : public Interpreter, public GestureConsumer {
 public:
  FilterInterpreter(PropRegistry* prop_reg,
                    Interpreter* next,
                    Tracer* tracer,
                    bool force_logging)
      : Interpreter(prop_reg, tracer, force_logging) { next_.reset(next); }
  virtual ~FilterInterpreter() {}

  Json::Value EncodeCommonInfo();
  void Clear();

  virtual void Initialize(const HardwareProperties* hwprops,
                          Metrics* metrics, MetricsProperties* mprops,
                          GestureConsumer* consumer);

  virtual void ConsumeGesture(const Gesture& gesture);

  // Temporary method for transitioning the old gesture list to
  // multiple calls of ConsumeGesture.
  void ConsumeGestureList(Gesture* gesture);

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);
  virtual void HandleTimerImpl(stime_t now, stime_t* timeout);

  std::unique_ptr<Interpreter> next_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FilterInterpreter);
};
}  // namespace gestures

#endif  // GESTURES_FILTER_INTERPRETER_H__
