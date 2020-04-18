// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/updated_progress_marker_checker.h"

#include "components/browser_sync/profile_sync_service.h"
#include "components/sync/engine/cycle/sync_cycle_snapshot.h"

UpdatedProgressMarkerChecker::UpdatedProgressMarkerChecker(
    browser_sync::ProfileSyncService* service)
    : SingleClientStatusChangeChecker(service), weak_ptr_factory_(this) {
  // HasUnsyncedItemsForTest() posts a task to the sync thread which guarantees
  // that all tasks posted to the sync thread before this constructor have been
  // processed.
  service->HasUnsyncedItemsForTest(
      base::BindOnce(&UpdatedProgressMarkerChecker::GotHasUnsyncedItems,
                     weak_ptr_factory_.GetWeakPtr()));
}

UpdatedProgressMarkerChecker::~UpdatedProgressMarkerChecker() {}

bool UpdatedProgressMarkerChecker::IsExitConditionSatisfied() {
  if (!has_unsynced_items_.has_value()) {
    return false;
  }

  const syncer::SyncCycleSnapshot& snap = service()->GetLastCycleSnapshot();
  // Assuming the lack of ongoing remote changes, the progress marker can be
  // considered updated when:
  // 1. Progress markers are non-empty (which discards the default value for
  //    GetLastCycleSnapshot() prior to the first sync cycle).
  // 2. Our last sync cycle committed no changes (because commits are followed
  //    by the test-only 'self-notify' cycle).
  // 3. Sync is still active (e.g. no failures).
  // 4. No pending local changes (which will ultimately generate new progress
  //    markers once submitted to the server).
  return !snap.download_progress_markers().empty() &&
         snap.model_neutral_state().num_successful_commits == 0 &&
         service()->IsSyncActive() && !has_unsynced_items_.value();
}

void UpdatedProgressMarkerChecker::GotHasUnsyncedItems(
    bool has_unsynced_items) {
  has_unsynced_items_ = has_unsynced_items;
  CheckExitCondition();
}

std::string UpdatedProgressMarkerChecker::GetDebugMessage() const {
  return "Waiting for progress markers";
}

void UpdatedProgressMarkerChecker::OnSyncCycleCompleted(
    syncer::SyncService* sync) {
  // Ignore sync cycles that were started before our constructor posted
  // HasUnsyncedItemsForTest() to the sync thread.
  if (!has_unsynced_items_.has_value()) {
    return;
  }

  // Override |has_unsynced_items_| with the result of the sync cycle.
  const syncer::SyncCycleSnapshot& snap = service()->GetLastCycleSnapshot();
  has_unsynced_items_ = snap.has_remaining_local_changes();
  CheckExitCondition();
}
