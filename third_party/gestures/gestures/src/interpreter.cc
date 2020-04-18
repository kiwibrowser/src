// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/interpreter.h"

#include <cxxabi.h>
#include <string>

#include <json/value.h>
#include <json/writer.h>

#include "gestures/include/activity_log.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/logging.h"
#include "gestures/include/tracer.h"

using std::string;

namespace gestures {

Interpreter::Interpreter(PropRegistry* prop_reg,
                         Tracer* tracer,
                         bool force_logging)
    : requires_metrics_(false),
      initialized_(false),
      name_(NULL),
      tracer_(tracer) {
#ifdef DEEP_LOGS
  bool logging_enabled = true;
#else
  bool logging_enabled = force_logging;
#endif
  if (logging_enabled)
    log_.reset(new ActivityLog(prop_reg));
}

Interpreter::~Interpreter() {
  if (name_)
    free(const_cast<char*>(name_));
}

void Interpreter::Trace(const char* message, const char* name) {
  if (tracer_)
    tracer_->Trace(message, name);
}

void Interpreter::SyncInterpret(HardwareState* hwstate,
                                    stime_t* timeout) {
  AssertWithReturn(initialized_);
  if (log_.get() && hwstate) {
    Trace("log: start: ", "LogHardwareState");
    log_->LogHardwareState(*hwstate);
    Trace("log: end: ", "LogHardwareState");
  }
  if (own_metrics_)
    own_metrics_->Update(*hwstate);

  Trace("SyncInterpret: start: ", name());
  SyncInterpretImpl(hwstate, timeout);
  Trace("SyncInterpret: end: ", name());
  LogOutputs(NULL, timeout, "SyncLogOutputs");
}

void Interpreter::HandleTimer(stime_t now, stime_t* timeout) {
  AssertWithReturn(initialized_);
  if (log_.get()) {
    Trace("log: start: ", "LogTimerCallback");
    log_->LogTimerCallback(now);
    Trace("log: end: ", "LogTimerCallback");
  }
  Trace("HandleTimer: start: ", name());
  HandleTimerImpl(now, timeout);
  Trace("HandleTimer: end: ", name());
  LogOutputs(NULL, timeout, "TimerLogOutputs");
}

void Interpreter::ProduceGesture(const Gesture& gesture) {
  AssertWithReturn(initialized_);
  LogOutputs(&gesture, NULL, "ProduceGesture");
  consumer_->ConsumeGesture(gesture);
}

void Interpreter::Initialize(const HardwareProperties* hwprops,
                             Metrics* metrics,
                             MetricsProperties* mprops,
                             GestureConsumer* consumer) {
  if (log_.get() && hwprops) {
    Trace("log: start: ", "SetHardwareProperties");
    log_->SetHardwareProperties(*hwprops);
    Trace("log: end: ", "SetHardwareProperties");
  }

  metrics_ = metrics;
  if (requires_metrics_ && metrics == NULL) {
    own_metrics_.reset(new Metrics(mprops));
    metrics_ = own_metrics_.get();
  }

  hwprops_ = hwprops;
  consumer_ = consumer;
  initialized_ = true;
}

Json::Value Interpreter::EncodeCommonInfo() {
  Json::Value root = log_.get() ?
      log_->EncodeCommonInfo() : Json::Value(Json::objectValue);
  root[ActivityLog::kKeyInterpreterName] = Json::Value(string(name()));
  return root;
}

std::string Interpreter::Encode() {
  Json::Value root = EncodeCommonInfo();
  if (log_.get())
    log_->AddEncodeInfo(&root);

  std::string out = root.toStyledString();
  return out;
}

void Interpreter::InitName() {
  if (!name_) {
    int status;
    char* full_name = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);
    if (full_name == NULL) {
      if (status == -1)
        Err("Memory allocation failed");
      else if (status == -2)
        Err("Mangled_name is not a valid name");
      else if (status == -3)
        Err("One of the arguments is invalid");
      return;
    }
    // the return value of abi::__cxa_demangle(...) is gestures::XXXInterpreter
    char* last_colon = strrchr(full_name, ':');
    char* class_name;
    if (last_colon)
      class_name = last_colon + 1;
    else
      class_name = full_name;
    name_ = strdup(class_name);
    free(full_name);
  }
}

void Interpreter::LogOutputs(const Gesture* result,
                             stime_t* timeout,
                             const char* action) {
  if (!log_.get())
    return;
  Trace("log: start: ", action);
  if (result)
    log_->LogGesture(*result);
  if (timeout && *timeout >= 0.0)
    log_->LogCallbackRequest(*timeout);
  Trace("log: end: ", action);
}
}  // namespace gestures
