// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/sync_change_processor_wrapper_for_test.h"

namespace syncer {

SyncChangeProcessorWrapperForTest::SyncChangeProcessorWrapperForTest(
    SyncChangeProcessor* wrapped)
    : wrapped_(wrapped) {
  DCHECK(wrapped_);
}

SyncChangeProcessorWrapperForTest::~SyncChangeProcessorWrapperForTest() {}

SyncError SyncChangeProcessorWrapperForTest::ProcessSyncChanges(
    const base::Location& from_here,
    const SyncChangeList& change_list) {
  return wrapped_->ProcessSyncChanges(from_here, change_list);
}

SyncDataList SyncChangeProcessorWrapperForTest::GetAllSyncData(
    ModelType type) const {
  return wrapped_->GetAllSyncData(type);
}

}  // namespace syncer
