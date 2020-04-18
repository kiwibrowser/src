// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_USER_EVENTS_USER_EVENT_SYNC_BRIDGE_H_
#define COMPONENTS_SYNC_USER_EVENTS_USER_EVENT_SYNC_BRIDGE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/optional.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/model/model_type_change_processor.h"
#include "components/sync/model/model_type_store.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "components/sync/user_events/global_id_mapper.h"

namespace syncer {

class UserEventSyncBridge : public ModelTypeSyncBridge {
 public:
  UserEventSyncBridge(
      OnceModelTypeStoreFactory store_factory,
      std::unique_ptr<ModelTypeChangeProcessor> change_processor,
      GlobalIdMapper* global_id_mapper,
      SyncService* sync_service);
  ~UserEventSyncBridge() override;

  // ModelTypeSyncBridge implementation.
  void OnSyncStarting() override;
  std::unique_ptr<MetadataChangeList> CreateMetadataChangeList() override;
  base::Optional<ModelError> MergeSyncData(
      std::unique_ptr<MetadataChangeList> metadata_change_list,
      EntityChangeList entity_data) override;
  base::Optional<ModelError> ApplySyncChanges(
      std::unique_ptr<MetadataChangeList> metadata_change_list,
      EntityChangeList entity_changes) override;
  void GetData(StorageKeyList storage_keys, DataCallback callback) override;
  void GetAllData(DataCallback callback) override;
  std::string GetClientTag(const EntityData& entity_data) override;
  std::string GetStorageKey(const EntityData& entity_data) override;
  DisableSyncResponse ApplyDisableSyncChanges(
      std::unique_ptr<MetadataChangeList> delete_metadata_change_list) override;

  void RecordUserEvent(std::unique_ptr<sync_pb::UserEventSpecifics> specifics);

 private:
  void RecordUserEventImpl(
      std::unique_ptr<sync_pb::UserEventSpecifics> specifics);
  // Record events in the deferred queue and clear the queue.
  void ProcessQueuedEvents();

  void OnStoreCreated(const base::Optional<ModelError>& error,
                      std::unique_ptr<ModelTypeStore> store);
  void OnReadAllMetadata(const base::Optional<ModelError>& error,
                         std::unique_ptr<MetadataBatch> metadata_batch);
  void OnCommit(const base::Optional<ModelError>& error);
  void OnReadData(DataCallback callback,
                  const base::Optional<ModelError>& error,
                  std::unique_ptr<ModelTypeStore::RecordList> data_records,
                  std::unique_ptr<ModelTypeStore::IdList> missing_id_list);
  void OnReadAllData(DataCallback callback,
                     const base::Optional<ModelError>& error,
                     std::unique_ptr<ModelTypeStore::RecordList> data_records);
  void OnReadAllDataToDelete(
      std::unique_ptr<MetadataChangeList> delete_metadata_change_list,
      const base::Optional<ModelError>& error,
      std::unique_ptr<ModelTypeStore::RecordList> data_records);

  // Resubmit all the events persisted in the store to sync events, which were
  // preserved when sync was disabled. This may resubmit entities that the
  // processor already knows about (i.e. with metadata), but it is allowed.
  void ReadAllDataAndResubmit();
  void OnReadAllDataToResubmit(
      const base::Optional<ModelError>& error,
      std::unique_ptr<ModelTypeStore::RecordList> data_records);

  void HandleGlobalIdChange(int64_t old_global_id, int64_t new_global_id);

  std::string GetAuthenticatedAccountId() const;

  // Persistent storage for in flight events. Should remain quite small, as we
  // delete upon commit confirmation.
  std::unique_ptr<ModelTypeStore> store_;

  // Used to store important events while the store or change processor are not
  // ready. This currently only handles user consents.
  std::vector<std::unique_ptr<sync_pb::UserEventSpecifics>>
      deferred_user_events_while_initializing_;

  // The key is the global_id of the navigation the event is linked to.
  std::multimap<int64_t, sync_pb::UserEventSpecifics>
      in_flight_nav_linked_events_;

  GlobalIdMapper* global_id_mapper_;
  SyncService* sync_service_;

  bool is_sync_starting_or_started_;

  DISALLOW_COPY_AND_ASSIGN(UserEventSyncBridge);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_USER_EVENTS_USER_EVENT_SYNC_BRIDGE_H_
