// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/fake_model_type_change_processor.h"

#include <utility>

#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

FakeModelTypeChangeProcessor::FakeModelTypeChangeProcessor()
    : FakeModelTypeChangeProcessor(nullptr) {}

FakeModelTypeChangeProcessor::FakeModelTypeChangeProcessor(
    base::WeakPtr<ModelTypeControllerDelegate> delegate)
    : delegate_(delegate) {}

FakeModelTypeChangeProcessor::~FakeModelTypeChangeProcessor() {
  // If this fails we were expecting an error but never got one.
  EXPECT_FALSE(expect_error_);
}

void FakeModelTypeChangeProcessor::Put(
    const std::string& client_tag,
    std::unique_ptr<EntityData> entity_data,
    MetadataChangeList* metadata_change_list) {}

void FakeModelTypeChangeProcessor::Delete(
    const std::string& client_tag,
    MetadataChangeList* metadata_change_list) {}

void FakeModelTypeChangeProcessor::UpdateStorageKey(
    const EntityData& entity_data,
    const std::string& storage_key,
    MetadataChangeList* metadata_change_list) {}

void FakeModelTypeChangeProcessor::UntrackEntity(
    const EntityData& entity_data) {}

void FakeModelTypeChangeProcessor::OnModelStarting(
    ModelTypeSyncBridge* bridge) {}

void FakeModelTypeChangeProcessor::ModelReadyToSync(
    std::unique_ptr<MetadataBatch> batch) {}

bool FakeModelTypeChangeProcessor::IsTrackingMetadata() {
  return true;
}

void FakeModelTypeChangeProcessor::ReportError(const ModelError& error) {
  EXPECT_TRUE(expect_error_) << error.ToString();
  expect_error_ = false;
}

base::WeakPtr<ModelTypeControllerDelegate>
FakeModelTypeChangeProcessor::GetControllerDelegateOnUIThread() {
  return delegate_;
}

void FakeModelTypeChangeProcessor::ExpectError() {
  expect_error_ = true;
}

}  // namespace syncer
