// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/observers/sync_disable_observer.h"

#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "components/sync/driver/sync_token_status.h"
#include "components/sync/engine/connection_status.h"

namespace ukm {

const base::Feature kUkmCheckAuthErrorFeature{"UkmCheckAuthError",
                                              base::FEATURE_ENABLED_BY_DEFAULT};

namespace {

enum DisableInfo {
  DISABLED_BY_NONE,
  DISABLED_BY_HISTORY,
  DISABLED_BY_INITIALIZED,
  DISABLED_BY_HISTORY_INITIALIZED,
  DISABLED_BY_CONNECTED,
  DISABLED_BY_HISTORY_CONNECTED,
  DISABLED_BY_INITIALIZED_CONNECTED,
  DISABLED_BY_HISTORY_INITIALIZED_CONNECTED,
  DISABLED_BY_PASSPHRASE,
  DISABLED_BY_HISTORY_PASSPHRASE,
  DISABLED_BY_INITIALIZED_PASSPHRASE,
  DISABLED_BY_HISTORY_INITIALIZED_PASSPHRASE,
  DISABLED_BY_CONNECTED_PASSPHRASE,
  DISABLED_BY_HISTORY_CONNECTED_PASSPHRASE,
  DISABLED_BY_INITIALIZED_CONNECTED_PASSPHRASE,
  DISABLED_BY_HISTORY_INITIALIZED_CONNECTED_PASSPHRASE,
  MAX_DISABLE_INFO
};

void RecordDisableInfo(DisableInfo info) {
  UMA_HISTOGRAM_ENUMERATION("UKM.SyncDisable.Info", info, MAX_DISABLE_INFO);
}

}  // namespace

SyncDisableObserver::SyncDisableObserver()
    : sync_observer_(this),
      all_histories_enabled_(false),
      all_extensions_enabled_(false) {}

SyncDisableObserver::~SyncDisableObserver() {}

// static
SyncDisableObserver::SyncState SyncDisableObserver::GetSyncState(
    syncer::SyncService* sync_service) {
  syncer::SyncTokenStatus status = sync_service->GetSyncTokenStatus();
  SyncState state;
  state.history_enabled = sync_service->GetPreferredDataTypes().Has(
      syncer::HISTORY_DELETE_DIRECTIVES);
  state.extensions_enabled =
      sync_service->GetPreferredDataTypes().Has(syncer::EXTENSIONS);
  state.initialized = sync_service->IsEngineInitialized();
  state.connected = !base::FeatureList::IsEnabled(kUkmCheckAuthErrorFeature) ||
                    status.connection_status == syncer::CONNECTION_OK;
  state.passphrase_protected =
      state.initialized && sync_service->IsUsingSecondaryPassphrase();
  return state;
}

void SyncDisableObserver::ObserveServiceForSyncDisables(
    syncer::SyncService* sync_service) {
  previous_states_[sync_service] = GetSyncState(sync_service);
  sync_observer_.Add(sync_service);
  UpdateAllProfileEnabled(false);
}

void SyncDisableObserver::UpdateAllProfileEnabled(bool must_purge) {
  bool all_enabled = CheckSyncStateOnAllProfiles();
  bool all_extensions_enabled =
      all_enabled && CheckSyncStateForExtensionsOnAllProfiles();
  // Any change in sync settings needs to call OnSyncPrefsChanged so that the
  // new settings take effect.
  if (must_purge || (all_enabled != all_histories_enabled_) ||
      (all_extensions_enabled != all_extensions_enabled_)) {
    all_histories_enabled_ = all_enabled;
    all_extensions_enabled_ = all_extensions_enabled;
    OnSyncPrefsChanged(must_purge);
  }
}

bool SyncDisableObserver::CheckSyncStateOnAllProfiles() {
  if (previous_states_.empty())
    return false;
  for (const auto& kv : previous_states_) {
    const SyncDisableObserver::SyncState& state = kv.second;
    if (!state.history_enabled || !state.initialized || !state.connected ||
        state.passphrase_protected) {
      int disabled_by = 0;
      if (!state.history_enabled)
        disabled_by |= 1 << 0;
      if (!state.initialized)
        disabled_by |= 1 << 1;
      if (!state.connected)
        disabled_by |= 1 << 2;
      if (state.passphrase_protected)
        disabled_by |= 1 << 3;
      RecordDisableInfo(DisableInfo(disabled_by));
      return false;
    }
  }
  RecordDisableInfo(DISABLED_BY_NONE);
  return true;
}

bool SyncDisableObserver::CheckSyncStateForExtensionsOnAllProfiles() {
  if (previous_states_.empty())
    return false;
  for (const auto& kv : previous_states_) {
    const SyncDisableObserver::SyncState& state = kv.second;
    if (!state.extensions_enabled)
      return false;
  }
  return true;
}

void SyncDisableObserver::OnStateChanged(syncer::SyncService* sync) {
  DCHECK(base::ContainsKey(previous_states_, sync));
  SyncDisableObserver::SyncState state = GetSyncState(sync);
  const SyncDisableObserver::SyncState& previous_state = previous_states_[sync];
  bool must_purge =
      // Trigger a purge if history sync was disabled.
      (previous_state.history_enabled && !state.history_enabled) ||
      // Trigger a purge if engine has become disabled.
      (previous_state.initialized && !state.initialized) ||
      // Trigger a purge if the user added a passphrase.  Since we can't detect
      // the use of a passphrase while the engine is not initialized, we may
      // miss the transition if the user adds a passphrase in this state.
      (previous_state.initialized && state.initialized &&
       !previous_state.passphrase_protected && state.passphrase_protected);
  previous_states_[sync] = state;
  UpdateAllProfileEnabled(must_purge);
}

void SyncDisableObserver::OnSyncShutdown(syncer::SyncService* sync) {
  DCHECK(base::ContainsKey(previous_states_, sync));
  sync_observer_.Remove(sync);
  previous_states_.erase(sync);
  UpdateAllProfileEnabled(false);
}

bool SyncDisableObserver::SyncStateAllowsUkm() {
  return all_histories_enabled_;
}

bool SyncDisableObserver::SyncStateAllowsExtensionUkm() {
  return all_extensions_enabled_;
}

}  // namespace ukm
