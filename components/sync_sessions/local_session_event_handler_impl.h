// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SESSIONS_LOCAL_SESSION_EVENT_HANDLER_IMPL_H_
#define COMPONENTS_SYNC_SESSIONS_LOCAL_SESSION_EVENT_HANDLER_IMPL_H_

#include <memory>
#include <set>
#include <string>

#include "base/macros.h"
#include "base/time/time.h"
#include "components/sessions/core/session_id.h"
#include "components/sessions/core/session_types.h"
#include "components/sync_sessions/local_session_event_router.h"
#include "components/sync_sessions/synced_session.h"
#include "components/sync_sessions/task_tracker.h"

namespace sync_pb {
class SessionSpecifics;
class SessionTab;
}  // namespace sync_pb

namespace sync_sessions {

class SyncedSessionTracker;
class SyncedTabDelegate;

// Class responsible for propagating local session changes to the sessions
// model including SyncedSessionTracker (in-memory representation) as well as
// the persistency and sync layers (via delegate).
class LocalSessionEventHandlerImpl : public LocalSessionEventHandler {
 public:
  class WriteBatch {
   public:
    WriteBatch();
    virtual ~WriteBatch();
    virtual void Delete(int tab_node_id) = 0;
    virtual void Put(std::unique_ptr<sync_pb::SessionSpecifics> specifics) = 0;
    virtual void Commit() = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(WriteBatch);
  };

  class Delegate {
   public:
    virtual ~Delegate();
    virtual std::unique_ptr<WriteBatch> CreateLocalSessionWriteBatch() = 0;
    // Analogous to SessionsGlobalIdMapper.
    virtual void TrackLocalNavigationId(base::Time timestamp,
                                        int unique_id) = 0;
    // Analogous to the functions in FaviconCache.
    virtual void OnPageFaviconUpdated(const GURL& page_url) = 0;
    virtual void OnFaviconVisited(const GURL& page_url,
                                  const GURL& favicon_url) = 0;
  };

  // Raw pointers must not be null and all pointees except |*initial_batch| must
  // outlive this object. |*initial_batch| may or may not be initially empty
  // (depending on whether the caller wants to bundle together other writes).
  // This constructor populates |*initial_batch| to resync local window and tab
  // information, but does *not* Commit() the batch.
  LocalSessionEventHandlerImpl(Delegate* delegate,
                               SyncSessionsClient* sessions_client,
                               SyncedSessionTracker* session_tracker,
                               WriteBatch* initial_batch);
  ~LocalSessionEventHandlerImpl() override;

  // LocalSessionEventHandler implementation.
  void OnLocalTabModified(SyncedTabDelegate* modified_tab) override;
  void OnFaviconsChanged(const std::set<GURL>& page_urls,
                         const GURL& icon_url) override;

  // Returns tab specifics from |tab_delegate|. Exposed publicly for testing.
  sync_pb::SessionTab GetTabSpecificsFromDelegateForTest(
      const SyncedTabDelegate& tab_delegate) const;

 private:
  enum ReloadTabsOption { RELOAD_TABS, DONT_RELOAD_TABS };

  // Updates |session_tracker_| with tab_id<->tab_node_id association that the
  // delegate already knows about, while resolving conflicts if the delegate
  // reports conflicting sync IDs. This makes sure duplicate tab_node_id-s are
  // not assigned. On return, the following conditions are met:
  // 1. Delegate contains no duplicate sync IDs (tab_node_id).
  // 2. Delegate contains no sync-ID <-> tab_id association that the tracker
  //    doesn't know about (but not the opposite).
  void AssociateExistingSyncIds();

  void AssociateWindows(ReloadTabsOption option,
                        bool has_tabbed_window,
                        WriteBatch* batch);

  // Loads and reassociates the local tab referenced in |tab|.
  // |batch| must not be null. This function will append necessary
  // changes for processing later. Will only assign a new sync id if there is
  // a tabbed window, which results in failure for tabs without sync ids yet.
  void AssociateTab(SyncedTabDelegate* const tab,
                    bool has_tabbed_window,
                    WriteBatch* batch);

  // It's possible that when we associate windows, tabs aren't all loaded
  // into memory yet (e.g on android) and we don't have a WebContents. In this
  // case we can't do a full association, but we still want to update tab IDs
  // as they may have changed after a session was restored.  This method
  // compares new_tab_id and new_window_id against the previously persisted tab
  // ID and window ID (from our TabNodePool) and updates them if either differs.
  void AssociateRestoredPlaceholderTab(const SyncedTabDelegate& tab_delegate,
                                       SessionID new_tab_id,
                                       SessionID new_window_id,
                                       WriteBatch* batch);

  // Appends an ACTION_UPDATE for a sync tab entity onto |batch| to
  // reflect the contents of |tab|, given the tab node id |sync_id|.
  void AppendChangeForExistingTab(int sync_id,
                                  const sessions::SessionTab& tab,
                                  WriteBatch* batch) const;

  // Set |session_tab| from |tab_delegate|.
  sync_pb::SessionTab GetTabSpecificsFromDelegate(
      const SyncedTabDelegate& tab_delegate) const;

  // Updates task tracker with the navigations of |tab_delegate|.
  void UpdateTaskTracker(SyncedTabDelegate* const tab_delegate);

  // Update |tab_specifics| with the corresponding task ids.
  void WriteTasksIntoSpecifics(sync_pb::SessionTab* tab_specifics);

  // On Android, it's possible to not have any tabbed windows when only custom
  // tabs are currently open. This means that there is tab data that will be
  // restored later, but we cannot access it. This method is an elaborate way to
  // check if we're currently in that state or not.
  bool ScanForTabbedWindow();

  // Injected dependencies (not owned).
  Delegate* const delegate_;
  SyncSessionsClient* const sessions_client_;
  SyncedSessionTracker* const session_tracker_;

  // Tracks Chrome Tasks, which associates navigations, with tab and navigation
  // changes of current session.
  TaskTracker task_tracker_;

  std::string current_session_tag_;

  DISALLOW_COPY_AND_ASSIGN(LocalSessionEventHandlerImpl);
};

}  // namespace sync_sessions

#endif  // COMPONENTS_SYNC_SESSIONS_LOCAL_SESSION_EVENT_HANDLER_IMPL_H_
