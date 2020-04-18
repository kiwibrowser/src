// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_SESSIONS_TAB_DELEGATE_SYNC_ADAPTER_H_
#define COMPONENTS_NTP_SNIPPETS_SESSIONS_TAB_DELEGATE_SYNC_ADAPTER_H_

#include <vector>

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "components/ntp_snippets/sessions/foreign_sessions_suggestions_provider.h"
#include "components/sync/driver/sync_service_observer.h"

namespace syncer {
class SyncService;
}  // namespace syncer

namespace ntp_snippets {

// Adapter that sits on top of SyncService and OpenTabsUIDelegate and provides
// simplified notifications and accessors for foreign tabs data.
class TabDelegateSyncAdapter : public syncer::SyncServiceObserver,
                               public ForeignSessionsProvider {
 public:
  explicit TabDelegateSyncAdapter(syncer::SyncService* sync_service);
  ~TabDelegateSyncAdapter() override;

  // ForeignSessionsProvider implementation.
  bool HasSessionsData() override;
  std::vector<const sync_sessions::SyncedSession*> GetAllForeignSessions()
      override;
  void SubscribeForForeignTabChange(
      const base::Closure& change_callback) override;

 private:
  // syncer::SyncServiceObserver implementation.
  void OnStateChanged(syncer::SyncService* sync) override;
  void OnSyncConfigurationCompleted(syncer::SyncService* sync) override;
  void OnForeignSessionUpdated(syncer::SyncService* sync) override;

  void InvokeCallback();

  syncer::SyncService* sync_service_;
  base::Closure change_callback_;

  // Represents whether there was session data the last time |change_callback_|
  // was invoked.
  bool had_session_data_ = false;

  DISALLOW_COPY_AND_ASSIGN(TabDelegateSyncAdapter);
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_SESSIONS_TAB_DELEGATE_SYNC_ADAPTER_H_
