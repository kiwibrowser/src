// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/sync_engine_host_stub.h"

namespace syncer {

SyncEngineHostStub::SyncEngineHostStub() = default;
SyncEngineHostStub::~SyncEngineHostStub() = default;

void SyncEngineHostStub::OnEngineInitialized(
    ModelTypeSet initial_types,
    const WeakHandle<JsBackend>& js_backend,
    const WeakHandle<DataTypeDebugInfoListener>& debug_info_listener,
    const std::string& cache_guid,
    bool success) {}

void SyncEngineHostStub::OnSyncCycleCompleted(
    const SyncCycleSnapshot& snapshot) {}

void SyncEngineHostStub::OnProtocolEvent(const ProtocolEvent& event) {}

void SyncEngineHostStub::OnDirectoryTypeCommitCounterUpdated(
    ModelType type,
    const CommitCounters& counters) {}

void SyncEngineHostStub::OnDirectoryTypeUpdateCounterUpdated(
    ModelType type,
    const UpdateCounters& counters) {}

void SyncEngineHostStub::OnDatatypeStatusCounterUpdated(
    ModelType type,
    const StatusCounters& counters) {}

void SyncEngineHostStub::OnConnectionStatusChange(ConnectionStatus status) {}

void SyncEngineHostStub::OnMigrationNeededForTypes(ModelTypeSet types) {}

void SyncEngineHostStub::OnExperimentsChanged(const Experiments& experiments) {}

void SyncEngineHostStub::OnActionableError(const SyncProtocolError& error) {}

}  // namespace syncer
