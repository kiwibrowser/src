// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_PREFERENCES_PREF_MODEL_ASSOCIATOR_H_
#define COMPONENTS_SYNC_PREFERENCES_PREF_MODEL_ASSOCIATOR_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "components/sync/model/sync_data.h"
#include "components/sync/model/syncable_service.h"
#include "components/sync_preferences/synced_pref_observer.h"

namespace base {
class Value;
}

namespace sync_pb {
class PreferenceSpecifics;
}

namespace sync_preferences {

class PrefModelAssociatorClient;
class PrefServiceSyncable;

// Contains all preference sync related logic.
// TODO(sync): Merge this into PrefService once we separate the profile
// PrefService from the local state PrefService.
class PrefModelAssociator : public syncer::SyncableService {
 public:
  // Constructs a PrefModelAssociator initializing the |client_| and |type_|
  // instance variable. The |client| is not owned by this object and the caller
  // must ensure that it oulives the PrefModelAssociator.
  PrefModelAssociator(const PrefModelAssociatorClient* client,
                      syncer::ModelType type);
  ~PrefModelAssociator() override;

  // See description above field for details.
  bool models_associated() const { return models_associated_; }

  // syncer::SyncableService implementation.
  syncer::SyncDataList GetAllSyncData(syncer::ModelType type) const override;
  syncer::SyncError ProcessSyncChanges(
      const base::Location& from_here,
      const syncer::SyncChangeList& change_list) override;
  syncer::SyncMergeResult MergeDataAndStartSyncing(
      syncer::ModelType type,
      const syncer::SyncDataList& initial_sync_data,
      std::unique_ptr<syncer::SyncChangeProcessor> sync_processor,
      std::unique_ptr<syncer::SyncErrorFactory> sync_error_factory) override;
  void StopSyncing(syncer::ModelType type) override;

  // Register a preference with the specified name for syncing. We do not care
  // about the type at registration time, but when changes arrive from the
  // syncer, we check if they can be applied and if not drop them.
  // Note: This should only be called at profile startup time (before sync
  // begins).
  virtual void RegisterPref(const char* name);

  // Process a local preference change. This can trigger new SyncChanges being
  // sent to the syncer.
  virtual void ProcessPrefChange(const std::string& name);

  void SetPrefService(PrefServiceSyncable* pref_service);

  // Merges the local_value into the supplied server_value and returns
  // the result (caller takes ownership). If there is a conflict, the server
  // value always takes precedence. Note that only certain preferences will
  // actually be merged, all others will return a copy of the server value. See
  // the method's implementation for details.
  std::unique_ptr<base::Value> MergePreference(const std::string& name,
                                               const base::Value& local_value,
                                               const base::Value& server_value);

  // Fills |sync_data| with a sync representation of the preference data
  // provided.
  bool CreatePrefSyncData(const std::string& name,
                          const base::Value& value,
                          syncer::SyncData* sync_data) const;
  // Returns true if the pref under the given name is pulled down from sync.
  // Note this does not refer to SYNCABLE_PREF.
  bool IsPrefSynced(const std::string& name) const;

  // Returns true if the specified preference is registered for syncing.
  bool IsPrefRegistered(const std::string& name) const;

  // Adds a SyncedPrefObserver to watch for changes to a specific pref.
  void AddSyncedPrefObserver(const std::string& name,
                             SyncedPrefObserver* observer);

  // Removes a SyncedPrefObserver from a pref's list of observers.
  void RemoveSyncedPrefObserver(const std::string& name,
                                SyncedPrefObserver* observer);

  // Returns the PrefModelAssociatorClient for this object.
  const PrefModelAssociatorClient* client() const { return client_; }

  // Register callback method which will get called at the end of
  // PrefModelAssociator::MergeDataAndStartSyncing().
  void RegisterMergeDataFinishedCallback(const base::Closure& callback);

 private:
  friend class PrefServiceSyncableTest;

  // Create an association for a given preference. If |sync_pref| is valid,
  // signifying that sync has data for this preference, we reconcile their data
  // with ours and append a new UPDATE SyncChange to |sync_changes|. If
  // sync_pref is not set, we append an ADD SyncChange to |sync_changes| with
  // the current preference data.
  // Note: We do not modify the sync data for preferences that are either
  // controlled by policy (are not user modifiable) or have their default value
  // (are not user controlled).
  void InitPrefAndAssociate(const syncer::SyncData& sync_pref,
                            const std::string& pref_name,
                            syncer::SyncChangeList* sync_changes);

  static std::unique_ptr<base::Value> MergeListValues(
      const base::Value& from_value,
      const base::Value& to_value);

  static base::Value MergeDictionaryValues(const base::Value& from_value,
                                           const base::Value& to_value);

  // Extract preference value from sync specifics.
  static base::Value* ReadPreferenceSpecifics(
      const sync_pb::PreferenceSpecifics& specifics);

  // Do we have an active association between the preferences and sync models?
  // Set when start syncing, reset in StopSyncing. While this is not set, we
  // ignore any local preference changes (when we start syncing we will look
  // up the most recent values anyways).
  bool models_associated_;

  // Whether we're currently processing changes from the syncer. While this is
  // true, we ignore any local preference changes, since we triggered them.
  bool processing_syncer_changes_;

  // A set of preference names.
  typedef std::set<std::string> PreferenceSet;

  // All preferences that have registered as being syncable with this profile.
  PreferenceSet registered_preferences_;

  // The preferences that are currently synced (excludes those preferences
  // that have never had sync data and currently have default values or are
  // policy controlled).
  // Note: this set never decreases, only grows to eventually match
  // registered_preferences_ as more preferences are synced. It determines
  // whether a preference change should update an existing sync node or create
  // a new sync node.
  PreferenceSet synced_preferences_;

  // The PrefService we are syncing to.
  PrefServiceSyncable* pref_service_;

  // Sync's syncer::SyncChange handler. We push all our changes through this.
  std::unique_ptr<syncer::SyncChangeProcessor> sync_processor_;

  // Sync's error handler. We use this to create sync errors.
  std::unique_ptr<syncer::SyncErrorFactory> sync_error_factory_;

  // The datatype that this associator is responible for, either PREFERENCES or
  // PRIORITY_PREFERENCES.
  syncer::ModelType type_;

  void NotifySyncedPrefObservers(const std::string& path, bool from_sync) const;

  // Map prefs to lists of observers. Observers will receive notification when
  // a pref changes, including the detail of whether or not the change came
  // from sync.
  base::hash_map<std::string,
                 std::unique_ptr<base::ObserverList<SyncedPrefObserver>>>
      synced_pref_observers_;
  const PrefModelAssociatorClient* client_;  // Weak.

  std::vector<base::Closure> callback_list_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(PrefModelAssociator);
};

}  // namespace sync_preferences

#endif  // COMPONENTS_SYNC_PREFERENCES_PREF_MODEL_ASSOCIATOR_H_
