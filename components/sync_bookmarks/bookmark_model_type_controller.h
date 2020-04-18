// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BOOKMARKS_BOOKMARK_MODEL_TYPE_CONTROLLER_H_
#define COMPONENTS_SYNC_BOOKMARKS_BOOKMARK_MODEL_TYPE_CONTROLLER_H_

#include <memory>

#include "base/macros.h"
#include "components/sync/driver/data_type_controller.h"
#include "components/sync/engine/activation_context.h"

namespace syncer {
class SyncClient;
}  // namespace syncer

namespace sync_bookmarks {

class BookmarkModelTypeProcessor;

// A class that manages the startup and shutdown of bookmark sync implemented
// through USS APIs.
class BookmarkModelTypeController : public syncer::DataTypeController {
 public:
  explicit BookmarkModelTypeController(syncer::SyncClient* sync_client);
  ~BookmarkModelTypeController() override;

  // syncer::DataTypeController implementation.
  bool ShouldLoadModelBeforeConfigure() const override;
  void BeforeLoadModels(syncer::ModelTypeConfigurer* configurer) override;
  void LoadModels(const ModelLoadCallback& model_load_callback) override;
  void RegisterWithBackend(base::Callback<void(bool)> set_downloaded,
                           syncer::ModelTypeConfigurer* configurer) override;
  void StartAssociating(const StartCallback& start_callback) override;
  void ActivateDataType(syncer::ModelTypeConfigurer* configurer) override;
  void DeactivateDataType(syncer::ModelTypeConfigurer* configurer) override;
  void Stop() override;
  State state() const override;
  void GetAllNodes(const AllNodesCallback& callback) override;
  void GetStatusCounters(const StatusCountersCallback& callback) override;
  void RecordMemoryUsageHistogram() override;

 private:
  friend class BookmarkModelTypeControllerTest;

  // Returns true if both BookmarkModel and HistoryService are loaded.
  bool DependenciesLoaded();

  // Reads ModelTypeState from storage and creates BookmarkModelTypeProcessor.
  std::unique_ptr<syncer::ActivationContext> PrepareActivationContext();

  // SyncClient provides access to BookmarkModel, HistoryService and
  // SyncService.
  syncer::SyncClient* sync_client_;

  // State of this datatype controller.
  State state_;

  // BookmarkModelTypeProcessor handles communications between sync engine and
  // BookmarkModel/HistoryService.
  std::unique_ptr<BookmarkModelTypeProcessor> model_type_processor_;

  // This is a hack to prevent reconfigurations from crashing, because USS
  // activation is not idempotent. RegisterWithBackend only needs to actually do
  // something the first time after the type is enabled.
  // TODO(crbug.com/647505): Remove this once the DTM handles things better.
  bool activated_ = false;

  DISALLOW_COPY_AND_ASSIGN(BookmarkModelTypeController);
};

}  // namespace sync_bookmarks

#endif  // COMPONENTS_SYNC_BOOKMARKS_BOOKMARK_MODEL_TYPE_CONTROLLER_H_
