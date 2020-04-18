// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/filter_interpreter.h"

#include <json/value.h>

namespace gestures {

void FilterInterpreter::SyncInterpretImpl(HardwareState* hwstate,
                                          stime_t* timeout) {
  next_->SyncInterpret(hwstate, timeout);
}

void FilterInterpreter::HandleTimerImpl(stime_t now, stime_t* timeout) {
  next_->HandleTimer(now, timeout);
}

void FilterInterpreter::Initialize(const HardwareProperties* hwprops,
                                   Metrics* metrics,
                                   MetricsProperties* mprops,
                                   GestureConsumer* consumer) {
  Interpreter::Initialize(hwprops, metrics, mprops, consumer);
  if (next_)
    next_->Initialize(hwprops, metrics, mprops, this);
}

void FilterInterpreter::ConsumeGesture(const Gesture& gesture) {
  ProduceGesture(gesture);
}

Json::Value FilterInterpreter::EncodeCommonInfo() {
  Json::Value root = Interpreter::EncodeCommonInfo();
#ifdef DEEP_LOGS
  root[ActivityLog::kKeyNext] = next_->EncodeCommonInfo();
#endif
  return root;
}

void FilterInterpreter::Clear() {
  if (log_.get())
    log_->Clear();
  next_->Clear();
}
}  // namespace gestures
