// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/p2p_sync_refresher.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/sync/test/integration/sync_datatype_helper.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sync/engine/cycle/sync_cycle_snapshot.h"
#include "content/public/browser/notification_service.h"

P2PSyncRefresher::P2PSyncRefresher(
    Profile* profile,
    browser_sync::ProfileSyncService* sync_service)
    : profile_(profile), sync_service_(sync_service) {
  sync_service_->AddObserver(this);
}

P2PSyncRefresher::~P2PSyncRefresher() {
  sync_service_->RemoveObserver(this);
}

void P2PSyncRefresher::OnSyncCycleCompleted(syncer::SyncService* sync) {
  const syncer::SyncCycleSnapshot& snap = sync_service_->GetLastCycleSnapshot();
  bool is_notifiable_commit =
      (snap.model_neutral_state().num_successful_commits > 0);
  if (is_notifiable_commit) {
    syncer::ModelTypeSet model_types =
        snap.model_neutral_state().commit_request_types;
    SyncTest* test = sync_datatype_helper::test();
    for (int i = 0; i < test->num_clients(); ++i) {
      if (profile_ != test->GetProfile(i))
        test->TriggerSyncForModelTypes(i, model_types);
    }
  }
}
