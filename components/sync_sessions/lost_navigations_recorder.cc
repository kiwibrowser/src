// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_sessions/lost_navigations_recorder.h"

#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "components/sync/syncable/entry.h"

namespace sync_sessions {

LostNavigationsRecorder::LostNavigationsRecorder()
    : recorder_state_(RECORDER_STATE_NOT_SYNCING) {}

LostNavigationsRecorder::~LostNavigationsRecorder() {}

void LostNavigationsRecorder::OnLocalChange(
    const syncer::syncable::Entry* current_entry,
    const syncer::SyncChange& change) {
  if (change.sync_data().GetSpecifics().session().has_tab()) {
    TransitionState(current_entry->GetSyncing(),
                    current_entry->GetIsUnsynced());
  }
  RecordChange(change);
}

// Record a change either by adjusting our list of tabs or by recording the set
// of navigations in the updated tab.
void LostNavigationsRecorder::RecordChange(const syncer::SyncChange& change) {
  sync_pb::SessionSpecifics session_specifics =
      change.sync_data().GetSpecifics().session();
  if (session_specifics.has_header()) {
    DeleteTabs(session_specifics.header());
    return;
  } else if (!session_specifics.has_tab()) {
    // There isn't any data we care about if neither a tab or window is
    // modified.
    return;
  }
  sync_pb::SessionTab tab = session_specifics.tab();
  SessionID tab_id = SessionID::FromSerializedValue(tab.tab_id());

  IdSet& latest = latest_navigation_ids_[tab_id];
  latest.clear();

  for (sync_pb::TabNavigation nav : tab.navigation()) {
    id_type uid = nav.unique_id();
    // Only record an id if it's "new" i.e. larger than the largest seen for
    // that tab. If the id is smaller than this, it's not new; ids are generated
    // in increasing order.
    if (uid > max_recorded_for_tab_[tab_id]) {
      recorded_navigation_ids_[tab_id].insert(uid);
      max_recorded_for_tab_[tab_id] = uid;
    }
    latest.insert(nav.unique_id());
  }
}

void LostNavigationsRecorder::DeleteTabs(const sync_pb::SessionHeader& header) {
  std::set<SessionID> new_tab_ids;
  std::set<SessionID> current_tab_ids;
  // Find the set of tab ids that are still there after the deletion.
  for (sync_pb::SessionWindow window : header.window()) {
    for (auto tab_id : window.tab()) {
      new_tab_ids.insert(SessionID::FromSerializedValue(tab_id));
    }
  }
  for (auto pair : recorded_navigation_ids_) {
    current_tab_ids.insert(pair.first);
  }
  // The set of deleted tabs is the difference between the set of tabs before
  // the pending change and the set of tabs following the pending change.
  auto deleted_tabs =
      base::STLSetDifference<std::set<SessionID>>(current_tab_ids, new_tab_ids);
  for (SessionID tab_id : deleted_tabs) {
    recorded_navigation_ids_.erase(tab_id);
    latest_navigation_ids_.erase(tab_id);
    max_recorded_for_tab_.erase(tab_id);
  }
}

// Change the current state of the recorder, possibly triggering reconciliation,
// based on the status of the directory entry. Reconciliation triggers on the
// observation of two conditions.
// 1) The entry transitioning into the syncing state
// 2) If we miss the transition to syncing state, the entry transitioning into
// the synced state.
void LostNavigationsRecorder::TransitionState(bool is_syncing,
                                              bool is_unsynced) {
  switch (recorder_state_) {
    case RECORDER_STATE_NOT_SYNCING:
      // If a sync cycle is happening or already finished, reconcile.
      // It's possible that this will trigger reconciliation multiple times per
      // sync cycle; once per tab that finishes the cycle in the synced state
      // and the user performs a navigation in. In theory this will cause
      // under-counting, since reconciliation clears each tab's remembered set
      // of navigations. In practice the number of unique tabs written to in
      // between two adjacent sync cycles should be pretty low,
      // making the undercounting tolerable.
      if (is_syncing || !is_unsynced) {
        ReconcileLostNavs();
      }
      // If we're currently in a sync cycle, remember that.
      if (is_syncing) {
        recorder_state_ = RECORDER_STATE_SYNCING;
      }
      break;
    case RECORDER_STATE_SYNCING:
      if (!is_syncing) {
        recorder_state_ = RECORDER_STATE_NOT_SYNCING;
      }
      break;
  }
}

// Reconcile the number of "lost" navigations by checking all the unique ids we
// recorded against what was actually synced.
void LostNavigationsRecorder::ReconcileLostNavs() {
  for (auto pair : recorded_navigation_ids_) {
    SessionID tab_id = pair.first;
    IdSet& latest = latest_navigation_ids_[tab_id];
    IdSet& recorded = recorded_navigation_ids_[tab_id];
    if (recorded.size() < 1) {
      continue;
    }

    // The set of lost navigations is anything we recorded as new that's not
    // present in latest.
    IdSet lost_navs = base::STLSetDifference<IdSet>(recorded, latest);
    int quantity_lost = lost_navs.size();
    UMA_HISTOGRAM_COUNTS("Sync.LostNavigationCount", quantity_lost);
    recorded.clear();
  }
}
}  // namespace sync_sessions
