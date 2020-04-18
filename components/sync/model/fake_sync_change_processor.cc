// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/fake_sync_change_processor.h"

#include "components/sync/model/sync_change.h"
#include "components/sync/model/sync_data.h"

namespace syncer {

FakeSyncChangeProcessor::FakeSyncChangeProcessor() {}

FakeSyncChangeProcessor::~FakeSyncChangeProcessor() {}

SyncError FakeSyncChangeProcessor::ProcessSyncChanges(
    const base::Location& from_here,
    const SyncChangeList& change_list) {
  changes_.insert(changes_.end(), change_list.begin(), change_list.end());
  return SyncError();
}

SyncDataList FakeSyncChangeProcessor::GetAllSyncData(ModelType type) const {
  return data_;
}

SyncError FakeSyncChangeProcessor::UpdateDataTypeContext(
    ModelType type,
    ContextRefreshStatus refresh_status,
    const std::string& context) {
  context_ = context;
  return SyncError();
}

void FakeSyncChangeProcessor::AddLocalChangeObserver(
    LocalChangeObserver* observer) {}

void FakeSyncChangeProcessor::RemoveLocalChangeObserver(
    LocalChangeObserver* observer) {}

const SyncChangeList& FakeSyncChangeProcessor::changes() const {
  return changes_;
}

SyncChangeList& FakeSyncChangeProcessor::changes() {
  return changes_;
}

const SyncDataList& FakeSyncChangeProcessor::data() const {
  return data_;
}

SyncDataList& FakeSyncChangeProcessor::data() {
  return data_;
}

const std::string& FakeSyncChangeProcessor::context() const {
  return context_;
}

std::string& FakeSyncChangeProcessor::context() {
  return context_;
}

}  // namespace syncer
