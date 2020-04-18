// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/sessions/tab_delegate_sync_adapter.h"

#include "components/sync/driver/sync_service.h"
#include "components/sync_sessions/open_tabs_ui_delegate.h"

using syncer::SyncService;
using sync_sessions::OpenTabsUIDelegate;

namespace ntp_snippets {

TabDelegateSyncAdapter::TabDelegateSyncAdapter(SyncService* sync_service)
    : sync_service_(sync_service) {
  sync_service_->AddObserver(this);
}

TabDelegateSyncAdapter::~TabDelegateSyncAdapter() {
  sync_service_->RemoveObserver(this);
}

bool TabDelegateSyncAdapter::HasSessionsData() {
  // GetOpenTabsUIDelegate will be a nullptr if sync has not started, or if the
  // sessions data type is not enabled.
  return sync_service_->GetOpenTabsUIDelegate() != nullptr;
}

std::vector<const sync_sessions::SyncedSession*>
TabDelegateSyncAdapter::GetAllForeignSessions() {
  std::vector<const sync_sessions::SyncedSession*> sessions;
  OpenTabsUIDelegate* delegate = sync_service_->GetOpenTabsUIDelegate();
  if (delegate != nullptr) {
    // The return bool from GetAllForeignSessions(...) is ignored.
    delegate->GetAllForeignSessions(&sessions);
  }
  return sessions;
}

void TabDelegateSyncAdapter::SubscribeForForeignTabChange(
    const base::Closure& change_callback) {
  DCHECK(change_callback_.is_null());
  change_callback_ = change_callback;
}

void TabDelegateSyncAdapter::OnStateChanged(syncer::SyncService* sync) {
  // OnStateChanged gets called very frequently, and usually is not important.
  // But there are some events, like disabling sync and signing out, that are
  // only captured through OnStateChange. In an attempt to send as few messages
  // as possible, track if there was session data, and always/only invoke the
  // callback when transitioning between states. This will also capture the case
  // when Open Tab is added/removed from syncing types. Note that this requires
  // the object behind GetOpenTabsUIDelegate() to have its real data when it
  // becomes available. Otherwise we might transition to think we have session
  // data, but invoke our callback while the GetOpenTabsUIDelegate() returns bad
  // results. Fortunately, this isn't a problem. GetOpenTabsUIDelegate() is
  // guarded by verifying the data type is RUNNING, which always means the
  // sessions merge has already happened.
  if (had_session_data_ != HasSessionsData()) {
    InvokeCallback();
  }
}

void TabDelegateSyncAdapter::OnSyncConfigurationCompleted(
    syncer::SyncService* sync) {
  // Ignored. This event can let us know when the set of enabled data types
  // change. However, we want to avoid useless notifications as much as
  // possible, and all of the information captured in this event will also be
  // covered by OnStateChange.
}

void TabDelegateSyncAdapter::OnForeignSessionUpdated(
    syncer::SyncService* sync) {
  // Foreign tab data changed, always invoke the callback to generate new
  // suggestions. Interestingly, this is only triggered after sync model type
  // apply, not after merge. The merge case should always be handled by
  // OnStateChange.
  InvokeCallback();
}

void TabDelegateSyncAdapter::InvokeCallback() {
  had_session_data_ = HasSessionsData();
  if (!change_callback_.is_null()) {
    change_callback_.Run();
  }
}

}  // namespace ntp_snippets
