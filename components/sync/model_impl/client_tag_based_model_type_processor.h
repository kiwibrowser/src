// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_MODEL_IMPL_CLIENT_TAG_BASED_MODEL_TYPE_PROCESSOR_H_
#define COMPONENTS_SYNC_MODEL_IMPL_CLIENT_TAG_BASED_MODEL_TYPE_PROCESSOR_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/sequence_checker.h"
#include "components/sync/base/model_type.h"
#include "components/sync/engine/cycle/status_counters.h"
#include "components/sync/engine/model_type_processor.h"
#include "components/sync/engine/non_blocking_sync_common.h"
#include "components/sync/model/conflict_resolution.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/metadata_change_list.h"
#include "components/sync/model/model_error.h"
#include "components/sync/model/model_type_change_processor.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "components/sync/protocol/model_type_state.pb.h"
#include "components/sync/protocol/sync.pb.h"

namespace syncer {

class CommitQueue;
class ProcessorEntityTracker;

// A sync component embedded on the model type's thread that tracks entity
// metadata in the model store and coordinates communication between sync and
// model type threads. See
// //docs/sync/uss/client_tag_based_model_type_processor.md for a more thorough
// description.
class ClientTagBasedModelTypeProcessor : public ModelTypeProcessor,
                                         public ModelTypeChangeProcessor,
                                         public ModelTypeControllerDelegate {
 public:
  ClientTagBasedModelTypeProcessor(ModelType type,
                                   const base::RepeatingClosure& dump_stack);
  // Used only for unit-tests.
  ClientTagBasedModelTypeProcessor(ModelType type,
                                   const base::RepeatingClosure& dump_stack,
                                   bool commit_only);
  ~ClientTagBasedModelTypeProcessor() override;

  // Returns true if the handshake with sync thread is complete.
  bool IsConnected() const;

  // ModelTypeChangeProcessor implementation.
  void Put(const std::string& storage_key,
           std::unique_ptr<EntityData> entity_data,
           MetadataChangeList* metadata_change_list) override;
  void Delete(const std::string& storage_key,
              MetadataChangeList* metadata_change_list) override;
  void UpdateStorageKey(const EntityData& entity_data,
                        const std::string& storage_key,
                        MetadataChangeList* metadata_change_list) override;
  void UntrackEntity(const EntityData& entity_data) override;
  void OnModelStarting(ModelTypeSyncBridge* bridge) override;
  void ModelReadyToSync(std::unique_ptr<MetadataBatch> batch) override;
  bool IsTrackingMetadata() override;
  void ReportError(const ModelError& error) override;
  base::WeakPtr<ModelTypeControllerDelegate> GetControllerDelegateOnUIThread()
      override;

  // ModelTypeProcessor implementation.
  void ConnectSync(std::unique_ptr<CommitQueue> worker) override;
  void DisconnectSync() override;
  void GetLocalChanges(size_t max_entries,
                       const GetLocalChangesCallback& callback) override;
  void OnCommitCompleted(const sync_pb::ModelTypeState& type_state,
                         const CommitResponseDataList& response_list) override;
  void OnUpdateReceived(const sync_pb::ModelTypeState& type_state,
                        const UpdateResponseDataList& updates) override;

  // ModelTypeControllerDelegate implementation.
  void OnSyncStarting(const ModelErrorHandler& error_handler,
                      StartCallback callback) override;
  void DisableSync() override;
  void GetAllNodesForDebugging(AllNodesCallback callback) override;
  void GetStatusCountersForDebugging(StatusCountersCallback callback) override;
  void RecordMemoryUsageHistogram() override;

  // Returns the estimate of dynamically allocated memory in bytes.
  size_t EstimateMemoryUsage() const;

  bool HasLocalChangesForTest() const;

 private:
  friend class ModelTypeDebugInfo;
  friend class ClientTagBasedModelTypeProcessorTest;

  // Returns true if the model is ready or encountered an error.
  bool IsModelReadyOrError() const;

  // Whether the processor is allowing changes to its model type. If this is
  // false, the bridge should not allow any changes to its data.
  bool IsAllowingChanges() const;

  // If preconditions are met, inform sync that we are ready to connect.
  void ConnectIfReady();

  // Helper function to process the update for a single entity. If a local data
  // change is required, it will be added to |entity_changes|. The return value
  // is the tracker for this entity, or nullptr if the update should be ignored.
  ProcessorEntityTracker* ProcessUpdate(const UpdateResponseData& update,
                                        EntityChangeList* entity_changes);

  // Resolve a conflict between |update| and the pending commit in |entity|.
  ConflictResolution::Type ResolveConflict(const UpdateResponseData& update,
                                           ProcessorEntityTracker* entity,
                                           EntityChangeList* changes);

  // Recommit all entities for encryption except those in |already_updated|.
  void RecommitAllForEncryption(std::unordered_set<std::string> already_updated,
                                MetadataChangeList* metadata_changes);

  // Handle the first update received from the server after being enabled.
  void OnInitialUpdateReceived(const sync_pb::ModelTypeState& type_state,
                               const UpdateResponseDataList& updates);

  // ModelTypeSyncBridge::GetData() callback for pending loading data upon
  // GetLocalChanges call.
  void OnPendingDataLoaded(size_t max_entries,
                           const GetLocalChangesCallback& callback,
                           std::unique_ptr<DataBatch> data_batch);

  // Caches EntityData from the |data_batch| in the entity trackers.
  void ConsumeDataBatch(std::unique_ptr<DataBatch> data_batch);

  // Prepares Commit requests and passes them to the GetLocalChanges callback.
  void CommitLocalChanges(size_t max_entries,
                          const GetLocalChangesCallback& callback);

  // Nudges worker if there are any local entities to be committed.
  void NudgeForCommitIfNeeded();

  // Returns true if there are any local entities to be committed.
  bool HasLocalChanges() const;

  // Computes the client tag hash for the given client |tag|.
  std::string GetHashForTag(const std::string& tag);

  // Looks up the client tag hash for the given |storage_key|, and regenerates
  // with |data| if the lookup finds nothing. Does not update the storage key to
  // client tag hash mapping.
  std::string GetClientTagHash(const std::string& storage_key,
                               const EntityData& data);

  // Gets the entity for the given storage key, or null if there isn't one.
  ProcessorEntityTracker* GetEntityForStorageKey(
      const std::string& storage_key);

  // Gets the entity for the given tag hash, or null if there isn't one.
  ProcessorEntityTracker* GetEntityForTagHash(const std::string& tag_hash);

  // Create an entity in the entity map for |storage_key| and return a pointer
  // to it.
  // Requires that no entity for |storage_key| already exists in the map.
  ProcessorEntityTracker* CreateEntity(const std::string& storage_key,
                                       const EntityData& data);

  // Version of the above that generates a tag for |data|.
  ProcessorEntityTracker* CreateEntity(const EntityData& data);

  // Returns true if all processor entity trackers have non-empty storage keys.
  bool AllStorageKeysPopulated() const;

  // Expires entries according to garbage collection directives.
  void ExpireEntriesIfNeeded(
      const sync_pb::DataTypeProgressMarker& progress_marker);

  // Clear metadata for the entries in |storage_key_to_be_deleted|.
  void ClearMetadataForEntries(
      const std::vector<std::string>& storage_key_to_be_deleted,
      MetadataChangeList* metadata_changes);

  // Tombstones all entries whose versions are older than
  // |version_watermark| unless they are unsynced.
  void ExpireEntriesByVersion(int64_t version_watermark,
                              MetadataChangeList* metadata_changes);

  // Tombstones all entries whose ages are older than
  // |age_watermark_in_days| unless they are unsynced.
  void ExpireEntriesByAge(int32_t age_watermark_in_days,
                          MetadataChangeList* metadata_changes);

  // If the number of |entities_| exceeds |max_number_of_items|, the
  // processor will tombstone the extra sync entities based on the LRU rule.
  void ExpireEntriesByItemLimit(int32_t max_number_of_items,
                                MetadataChangeList* metadata_changes);

  // Removes entity tracker and clears metadata for entity from
  // MetadataChangeList.
  void RemoveEntity(ProcessorEntityTracker* entity,
                    MetadataChangeList* metadata_change_list);

  // Resets the internal state of the processor to the one right after calling
  // the ctor (with the exception of |bridge_| which remains intact).
  // TODO(jkrcal): Replace the helper function by grouping the state naturally
  // into a few structs / nested classes so that the state can be reset by
  // resetting these structs.
  void ResetState();

  // Adds metadata to all data returned by the bridge.
  // TODO(jkrcal): Mark as const (together with functions it depends on such as
  // GetEntityForStorageKey, GetEntityForTagHash and maybe more).
  void MergeDataWithMetadataForDebugging(AllNodesCallback callback,
                                         std::unique_ptr<DataBatch> batch);

  /////////////////////
  // Processor state //
  /////////////////////

  // The model type this object syncs.
  const ModelType type_;

  // The model type metadata (progress marker, initial sync done, etc).
  sync_pb::ModelTypeState model_type_state_;

  // ModelTypeSyncBridge linked to this processor. The bridge owns this
  // processor instance so the pointer should never become invalid.
  ModelTypeSyncBridge* bridge_;

  // Function to capture and upload a stack trace when an error occurs.
  const base::RepeatingClosure dump_stack_;

  /////////////////
  // Model state //
  /////////////////

  // The first model error that occurred, if any. Stored to track model state
  // and so it can be passed to sync if it happened prior to sync being ready.
  base::Optional<ModelError> model_error_;

  // Whether the model has initialized its internal state for sync (and provided
  // metadata).
  bool model_ready_to_sync_ = false;

  ////////////////
  // Sync state //
  ////////////////

  // Stores the start callback in between OnSyncStarting() and ReadyToConnect().
  StartCallback start_callback_;

  // The callback used for informing sync of errors; will be non-null after
  // OnSyncStarting has been called.
  ModelErrorHandler error_handler_;

  // Reference to the CommitQueue.
  //
  // The interface hides the posting of tasks across threads as well as the
  // CommitQueue's implementation.  Both of these features are
  // useful in tests.
  std::unique_ptr<CommitQueue> worker_;

  //////////////////
  // Entity state //
  //////////////////

  // A map of client tag hash to sync entities known to this processor. This
  // should contain entries and metadata for most everything, although the
  // entities may not always contain model type data/specifics.
  std::map<std::string, std::unique_ptr<ProcessorEntityTracker>> entities_;

  // The bridge wants to communicate entirely via storage keys that it is free
  // to define and can understand more easily. All of the sync machinery wants
  // to use client tag hash. This mapping allows us to convert from storage key
  // to client tag hash. The other direction can use |entities_|.
  // Entity is temporarily not included in this map for the duration of
  // MergeSyncData/ApplySyncChanges call when the bridge doesn't support
  // GetStorageKey(). In this case the bridge is responsible for updating
  // storage key with UpdateStorageKey() call from within
  // MergeSyncData/ApplySyncChanges.
  std::map<std::string, std::string> storage_key_to_tag_hash_;

  // If the processor should behave as if |type_| is one of the commit only
  // model types. For this processor, being commit only means that on commit
  // confirmation, we should delete local data, because the model side never
  // intends to read it. This includes both data and metadata.
  const bool commit_only_;

  // The version which processor already ran garbage collection against on.
  // Cache this value is for saving resource purpose(ex. cpu, battery), so
  // processor only run on each version once.
  int64_t cached_gc_directive_version_;

  // The day which processor already ran garbage collection against on.
  // Cache this value is for saving resource purpose(ex. cpu, battery), we round
  // up garbage collection age to day, so we only run GC once a day if server
  // did not change the age out days.
  base::Time cached_gc_directive_aged_out_day_;

  SEQUENCE_CHECKER(sequence_checker_);

  // WeakPtrFactory for this processor which will be sent to sync thread.
  base::WeakPtrFactory<ClientTagBasedModelTypeProcessor> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ClientTagBasedModelTypeProcessor);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_MODEL_IMPL_CLIENT_TAG_BASED_MODEL_TYPE_PROCESSOR_H_
