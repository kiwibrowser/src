// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UKM_OBSERVERS_SYNC_DISABLE_OBSERVER_H_
#define COMPONENTS_UKM_OBSERVERS_SYNC_DISABLE_OBSERVER_H_

#include <map>

#include "base/scoped_observer.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_service_observer.h"

namespace ukm {

// Observes the state of a set of SyncServices for changes to history sync
// preferences.  This is for used to trigger purging of local state when
// sync is disabled on a profile and disabling recording when any non-syncing
// profiles are active.
class SyncDisableObserver : public syncer::SyncServiceObserver {
 public:
  SyncDisableObserver();
  ~SyncDisableObserver() override;

  // Starts observing a service for sync disables.
  void ObserveServiceForSyncDisables(syncer::SyncService* sync_service);

  // Returns true iff sync is in a state that allows UKM to be enabled.
  // This means that for all profiles, sync is initialized, connected, has the
  // HISTORY_DELETE_DIRECTIVES data type enabled, and does not have a secondary
  // passphrase enabled.
  virtual bool SyncStateAllowsUkm();

  // Returns true iff sync is in a state that allows UKM to capture extensions.
  // This means that all profiles have EXTENSIONS data type enabled for syncing.
  virtual bool SyncStateAllowsExtensionUkm();

 protected:
  // Called after state changes and some profile has sync disabled.
  // If |must_purge| is true, sync was disabled for some profile, and
  // local data should be purged.
  virtual void OnSyncPrefsChanged(bool must_purge) = 0;

 private:
  // syncer::SyncServiceObserver:
  void OnStateChanged(syncer::SyncService* sync) override;
  void OnSyncShutdown(syncer::SyncService* sync) override;

  // Recomputes all_profiles_enabled_ state from previous_states_;
  void UpdateAllProfileEnabled(bool must_purge);

  // Returns true iff all sync states in previous_states_ allow UKM.
  // If there are no profiles being observed, this returns false.
  bool CheckSyncStateOnAllProfiles();

  // Returns true iff all sync states in previous_states_ allow extension UKM.
  // If there are no profiles being observed, this returns false.
  bool CheckSyncStateForExtensionsOnAllProfiles();

  // Tracks observed history services, for cleanup.
  ScopedObserver<syncer::SyncService, syncer::SyncServiceObserver>
      sync_observer_;

  // State data about sync services that we need to remember.
  struct SyncState {
    // If the user has history sync enabled.
    bool history_enabled = false;
    // If the user has extension sync enabled.
    bool extensions_enabled = false;
    // Whether the sync service has been initialized.
    bool initialized = false;
    // Whether the sync service is active and operational.
    bool connected = false;
    // Whether user data is hidden by a secondary passphrase.
    // This is not valid if the state is not initialized.
    bool passphrase_protected = false;
  };

  // Gets the current state of a SyncService.
  static SyncState GetSyncState(syncer::SyncService* sync);

  // The list of services that had sync enabled when we last checked.
  std::map<syncer::SyncService*, SyncState> previous_states_;

  // Tracks if history sync was enabled on all profiles after the last state
  // change.
  bool all_histories_enabled_;

  // Tracks if extension sync was enabled on all profiles after the last state
  // change.
  bool all_extensions_enabled_;

  DISALLOW_COPY_AND_ASSIGN(SyncDisableObserver);
};

}  // namespace ukm

#endif  // COMPONENTS_UKM_OBSERVERS_SYNC_DISABLE_OBSERVER_H_
