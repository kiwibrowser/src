// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SESSIONS_LOST_NAVIGATIONS_RECORDER_H_
#define COMPONENTS_SYNC_SESSIONS_LOST_NAVIGATIONS_RECORDER_H_

#include <map>
#include <set>

#include "base/macros.h"
#include "components/sessions/core/session_id.h"
#include "components/sync/model/local_change_observer.h"
#include "components/sync/model/sync_change.h"
#include "components/sync/protocol/session_specifics.pb.h"

namespace sync_sessions {

// Recorder class that tracks the number of navigations written locally that
// aren't synced. This is done by recording set of locally observed navigations
// and  reconciling these sets against what was ultimately synced. These
// navigations ultimately feed chrome history, so losing them prevents them from
// being reflected in the history page.
class LostNavigationsRecorder : public syncer::LocalChangeObserver {
 public:
  using id_type = int32_t;
  using IdSet = std::set<id_type>;
  enum RecorderState { RECORDER_STATE_NOT_SYNCING, RECORDER_STATE_SYNCING };

  LostNavigationsRecorder();
  ~LostNavigationsRecorder() override;

  // syncer::LocalChangeObserver implementation.
  void OnLocalChange(const syncer::syncable::Entry* current_entry,
                     const syncer::SyncChange& change) override;

 private:
  void RecordChange(const syncer::SyncChange& change);
  void DeleteTabs(const sync_pb::SessionHeader& header);
  void TransitionState(bool is_syncing, bool is_unsynced);
  void ReconcileLostNavs();

  // State that records whether the most recently observed directory state was
  // syncing or not syncing.
  RecorderState recorder_state_;

  // The set of all navigation ids we've observed for each tab_id since the last
  // sync.
  std::map<SessionID, IdSet> recorded_navigation_ids_;
  // The set of navigation ids most recently recorded for each tab_id.
  std::map<SessionID, IdSet> latest_navigation_ids_;
  // The maximum navigation_id recorded for each tab_id.
  std::map<SessionID, id_type> max_recorded_for_tab_;
  DISALLOW_COPY_AND_ASSIGN(LostNavigationsRecorder);
};
};  // namespace sync_sessions

#endif  // COMPONENTS_SYNC_SESSIONS_LOST_NAVIGATIONS_RECORDER_H_
