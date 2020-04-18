// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_FAKE_GENERIC_CHANGE_PROCESSOR_H_
#define COMPONENTS_SYNC_DRIVER_FAKE_GENERIC_CHANGE_PROCESSOR_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/sync/base/model_type.h"
#include "components/sync/driver/generic_change_processor.h"
#include "components/sync/driver/generic_change_processor_factory.h"
#include "components/sync/driver/sync_api_component_factory.h"
#include "components/sync/model/sync_error.h"

namespace syncer {

// A fake GenericChangeProcessor that can return arbitrary values.
class FakeGenericChangeProcessor : public GenericChangeProcessor {
 public:
  FakeGenericChangeProcessor(ModelType type, SyncClient* sync_client);
  ~FakeGenericChangeProcessor() override;

  // Setters for GenericChangeProcessor implementation results.
  void set_sync_model_has_user_created_nodes(bool has_nodes);
  void set_sync_model_has_user_created_nodes_success(bool success);

  // GenericChangeProcessor implementations.
  SyncError ProcessSyncChanges(const base::Location& from_here,
                               const SyncChangeList& change_list) override;
  SyncError GetAllSyncDataReturnError(SyncDataList* data) const override;
  bool GetDataTypeContext(std::string* context) const override;
  int GetSyncCount() override;
  bool SyncModelHasUserCreatedNodes(bool* has_nodes) override;
  bool CryptoReadyIfNecessary() override;

 private:
  bool sync_model_has_user_created_nodes_;
  bool sync_model_has_user_created_nodes_success_;
};

// Define a factory for FakeGenericChangeProcessor for convenience.
class FakeGenericChangeProcessorFactory : public GenericChangeProcessorFactory {
 public:
  explicit FakeGenericChangeProcessorFactory(
      std::unique_ptr<FakeGenericChangeProcessor> processor);
  ~FakeGenericChangeProcessorFactory() override;
  std::unique_ptr<GenericChangeProcessor> CreateGenericChangeProcessor(
      ModelType type,
      UserShare* user_share,
      std::unique_ptr<DataTypeErrorHandler> error_handler,
      const base::WeakPtr<SyncableService>& local_service,
      const base::WeakPtr<SyncMergeResult>& merge_result,
      SyncClient* sync_client) override;

 private:
  std::unique_ptr<FakeGenericChangeProcessor> processor_;
  DISALLOW_COPY_AND_ASSIGN(FakeGenericChangeProcessorFactory);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_FAKE_GENERIC_CHANGE_PROCESSOR_H_
