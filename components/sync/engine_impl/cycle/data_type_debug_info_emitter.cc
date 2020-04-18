// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/cycle/data_type_debug_info_emitter.h"

#include "components/sync/engine/cycle/type_debug_info_observer.h"

namespace syncer {

DataTypeDebugInfoEmitter::DataTypeDebugInfoEmitter(
    ModelType type,
    base::ObserverList<TypeDebugInfoObserver>* observers)
    : type_(type), type_debug_info_observers_(observers) {}

DataTypeDebugInfoEmitter::~DataTypeDebugInfoEmitter() {}

const CommitCounters& DataTypeDebugInfoEmitter::GetCommitCounters() const {
  return commit_counters_;
}

CommitCounters* DataTypeDebugInfoEmitter::GetMutableCommitCounters() {
  return &commit_counters_;
}

void DataTypeDebugInfoEmitter::EmitCommitCountersUpdate() {
  for (auto& observer : *type_debug_info_observers_)
    observer.OnCommitCountersUpdated(type_, commit_counters_);
}

const UpdateCounters& DataTypeDebugInfoEmitter::GetUpdateCounters() const {
  return update_counters_;
}

UpdateCounters* DataTypeDebugInfoEmitter::GetMutableUpdateCounters() {
  return &update_counters_;
}

void DataTypeDebugInfoEmitter::EmitUpdateCountersUpdate() {
  for (auto& observer : *type_debug_info_observers_)
    observer.OnUpdateCountersUpdated(type_, update_counters_);
}

}  // namespace syncer
