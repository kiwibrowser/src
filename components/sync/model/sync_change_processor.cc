// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/sync_change_processor.h"

namespace syncer {

SyncChangeProcessor::SyncChangeProcessor() {}

SyncChangeProcessor::~SyncChangeProcessor() {}

SyncError SyncChangeProcessor::UpdateDataTypeContext(
    ModelType type,
    SyncChangeProcessor::ContextRefreshStatus refresh_status,
    const std::string& context) {
  // Do nothing.
  return SyncError();
}

void SyncChangeProcessor::AddLocalChangeObserver(
    LocalChangeObserver* observer) {
  NOTREACHED();
}

void SyncChangeProcessor::RemoveLocalChangeObserver(
    LocalChangeObserver* observer) {
  NOTREACHED();
}

}  // namespace syncer
