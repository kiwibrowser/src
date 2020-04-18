// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/shared_change_processor_ref.h"

namespace syncer {

SharedChangeProcessorRef::SharedChangeProcessorRef(
    const scoped_refptr<SharedChangeProcessor>& change_processor)
    : change_processor_(change_processor) {
  DCHECK(change_processor_);
}

SharedChangeProcessorRef::~SharedChangeProcessorRef() {}

SyncError SharedChangeProcessorRef::ProcessSyncChanges(
    const base::Location& from_here,
    const SyncChangeList& change_list) {
  return change_processor_->ProcessSyncChanges(from_here, change_list);
}

SyncDataList SharedChangeProcessorRef::GetAllSyncData(ModelType type) const {
  return change_processor_->GetAllSyncData(type);
}

SyncError SharedChangeProcessorRef::UpdateDataTypeContext(
    ModelType type,
    SyncChangeProcessor::ContextRefreshStatus refresh_status,
    const std::string& context) {
  return change_processor_->UpdateDataTypeContext(type, refresh_status,
                                                  context);
}

void SharedChangeProcessorRef::AddLocalChangeObserver(
    LocalChangeObserver* observer) {
  change_processor_->AddLocalChangeObserver(observer);
}

void SharedChangeProcessorRef::RemoveLocalChangeObserver(
    LocalChangeObserver* observer) {
  change_processor_->RemoveLocalChangeObserver(observer);
}

SyncError SharedChangeProcessorRef::CreateAndUploadError(
    const base::Location& from_here,
    const std::string& message) {
  return change_processor_->CreateAndUploadError(from_here, message);
}

}  // namespace syncer
