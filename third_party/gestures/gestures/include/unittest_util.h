// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GESTURES_UNITTEST_UTIL_H_
#define GESTURES_UNITTEST_UTIL_H_

#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/interpreter.h"

namespace gestures {

// A wrapper for interpreters in unit tests. Mimicks the old API and
// initializes the interpreter correctly.
class TestInterpreterWrapper : public GestureConsumer {
 public:
  explicit TestInterpreterWrapper(Interpreter* interpreter);
  TestInterpreterWrapper(Interpreter* interpreter,
                         const HardwareProperties* hwprops);

  void Reset(Interpreter* interpreter);
  // Takes ownership of mprops
  void Reset(Interpreter* interpreter, MetricsProperties* mprops);
  void Reset(Interpreter* interpreter, const HardwareProperties* hwprops);
  Gesture* SyncInterpret(HardwareState* state, stime_t* timeout);
  Gesture* HandleTimer(stime_t now, stime_t* timeout);
  virtual void ConsumeGesture(const Gesture& gs);

 private:
  Interpreter* interpreter_;
  const HardwareProperties* hwprops_;
  HardwareProperties dummy_;
  Gesture gesture_;
  std::unique_ptr<PropRegistry> prop_reg_;
  std::unique_ptr<MetricsProperties> mprops_;
};


}  // namespace gestures

#endif  // GESTURES_UNITTEST_UTIL_H_
