// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/fake_generic_change_processor.h"

#include <utility>

#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "components/sync/model/syncable_service.h"

namespace syncer {

FakeGenericChangeProcessor::FakeGenericChangeProcessor(ModelType type,
                                                       SyncClient* sync_client)
    : GenericChangeProcessor(type,
                             nullptr,
                             base::WeakPtr<SyncableService>(),
                             base::WeakPtr<SyncMergeResult>(),
                             nullptr,
                             sync_client),
      sync_model_has_user_created_nodes_(true),
      sync_model_has_user_created_nodes_success_(true) {}

FakeGenericChangeProcessor::~FakeGenericChangeProcessor() {}

void FakeGenericChangeProcessor::set_sync_model_has_user_created_nodes(
    bool has_nodes) {
  sync_model_has_user_created_nodes_ = has_nodes;
}
void FakeGenericChangeProcessor::set_sync_model_has_user_created_nodes_success(
    bool success) {
  sync_model_has_user_created_nodes_success_ = success;
}

SyncError FakeGenericChangeProcessor::ProcessSyncChanges(
    const base::Location& from_here,
    const SyncChangeList& change_list) {
  return SyncError();
}

SyncError FakeGenericChangeProcessor::GetAllSyncDataReturnError(
    SyncDataList* current_sync_data) const {
  return SyncError();
}

bool FakeGenericChangeProcessor::GetDataTypeContext(
    std::string* context) const {
  return false;
}

int FakeGenericChangeProcessor::GetSyncCount() {
  return 0;
}

bool FakeGenericChangeProcessor::SyncModelHasUserCreatedNodes(bool* has_nodes) {
  *has_nodes = sync_model_has_user_created_nodes_;
  return sync_model_has_user_created_nodes_success_;
}

bool FakeGenericChangeProcessor::CryptoReadyIfNecessary() {
  return true;
}

FakeGenericChangeProcessorFactory::FakeGenericChangeProcessorFactory(
    std::unique_ptr<FakeGenericChangeProcessor> processor)
    : processor_(std::move(processor)) {}

FakeGenericChangeProcessorFactory::~FakeGenericChangeProcessorFactory() {}

std::unique_ptr<GenericChangeProcessor>
FakeGenericChangeProcessorFactory::CreateGenericChangeProcessor(
    ModelType type,
    UserShare* user_share,
    std::unique_ptr<DataTypeErrorHandler> error_handler,
    const base::WeakPtr<SyncableService>& local_service,
    const base::WeakPtr<SyncMergeResult>& merge_result,
    SyncClient* sync_client) {
  return std::move(processor_);
}

}  // namespace syncer
