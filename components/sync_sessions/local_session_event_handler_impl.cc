// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_sessions/local_session_event_handler_impl.h"

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "components/sync/model/sync_change.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync_sessions/sync_sessions_client.h"
#include "components/sync_sessions/synced_session_tracker.h"
#include "components/sync_sessions/synced_tab_delegate.h"
#include "components/sync_sessions/synced_window_delegate.h"
#include "components/sync_sessions/synced_window_delegates_getter.h"

namespace sync_sessions {
namespace {

using sessions::SerializedNavigationEntry;

// The maximum number of navigations in each direction we care to sync.
const int kMaxSyncNavigationCount = 6;

bool IsWindowSyncable(const SyncedWindowDelegate& window_delegate) {
  return window_delegate.ShouldSync() && window_delegate.GetTabCount() &&
         window_delegate.HasWindow();
}

// Each sync id should only ever be used once. Previously there existed a race
// condition which could cause them to be duplicated, see
// https://crbug.com/639009 for more information. This counts the number of
// times each id is used so that the second window/tab loop can act on every
// tab using duplicate ids. Lastly, it is important to note that this
// duplication scan is only checking the in-memory tab state. On Android, if
// we have no tabbed window, we may also have sync data with conflicting sync
// ids, but to keep this logic simple and less error prone, we do not attempt
// to do anything clever.
//
// Returns tabs grouped by sync id (tab_node_id).
std::map<int, std::vector<SyncedTabDelegate*>> GroupTabsBySyncId(
    const SyncedWindowDelegatesGetter::SyncedWindowDelegateMap& windows) {
  std::map<int, std::vector<SyncedTabDelegate*>> tabs_per_sync_id;
  int duplicate_count = 0;
  for (auto& window_iter_pair : windows) {
    const SyncedWindowDelegate* window_delegate = window_iter_pair.second;
    for (int j = 0; j < window_delegate->GetTabCount(); ++j) {
      SyncedTabDelegate* synced_tab = window_delegate->GetTabAt(j);
      if (!synced_tab ||
          synced_tab->GetSyncId() == TabNodePool::kInvalidTabNodeID) {
        continue;
      }
      std::vector<SyncedTabDelegate*>* synced_tabs =
          &tabs_per_sync_id[synced_tab->GetSyncId()];
      synced_tabs->push_back(synced_tab);
      if (synced_tabs->size() > 1) {
        // If an id is used more than twice, this count will be a bit odd,
        // but for our purposes, it will be sufficient.
        duplicate_count++;
      }
    }
  }

  if (duplicate_count > 0) {
    UMA_HISTOGRAM_COUNTS_100("Sync.SesssionsDuplicateSyncId", duplicate_count);
  }
  return tabs_per_sync_id;
}

// Function to resolve a conflict when multiple tabs reported by the delegate
// contain a shared sync ID (tab_node_id). |initially_associated_tab_id| can be
// invalid if |tab_node_id| is currently not associated in the tracker.
const SyncedTabDelegate* ResolveConflictingSyncId(
    int tab_node_id,
    SessionID initially_associated_tab_id,
    const std::vector<SyncedTabDelegate*>& synced_tabs) {
  DCHECK_GT(synced_tabs.size(), 1U);
  // If among the conflicting tabs, there's one which matches what sync knows
  // about, we should prefer that. This should be rare but is possible for
  // example if a Custom tab was being synced without a tabbed window, and
  // suddenly the tabbed window shows up with conflicting sync IDs.
  for (const SyncedTabDelegate* synced_tab : synced_tabs) {
    if (synced_tab->GetSessionId() == initially_associated_tab_id) {
      return synced_tab;
    }
  }
  // In doubt, prefer non-placeholder tabs, because we cannot do anything about
  // placeholder tabs that the tracker doesn't know about (because the delegate
  // does not provide enough information to start tracking it).
  for (const SyncedTabDelegate* synced_tab : synced_tabs) {
    if (!synced_tab->IsPlaceholderTab()) {
      // If multiple non-placeholder tabs have the same sync ID, choose among
      // them arbitrarily.
      return synced_tab;
    }
  }
  // We have multiple conflicting placeholder tabs, so choose arbitrarily.
  return synced_tabs.front();
}

}  // namespace

LocalSessionEventHandlerImpl::WriteBatch::WriteBatch() = default;

LocalSessionEventHandlerImpl::WriteBatch::~WriteBatch() = default;

LocalSessionEventHandlerImpl::Delegate::~Delegate() = default;

LocalSessionEventHandlerImpl::LocalSessionEventHandlerImpl(
    Delegate* delegate,
    SyncSessionsClient* sessions_client,
    SyncedSessionTracker* session_tracker,
    WriteBatch* initial_batch)
    : delegate_(delegate),
      sessions_client_(sessions_client),
      session_tracker_(session_tracker) {
  DCHECK(delegate);
  DCHECK(sessions_client);
  DCHECK(session_tracker);
  DCHECK(initial_batch);

  current_session_tag_ = session_tracker_->GetLocalSessionTag();
  DCHECK(!current_session_tag_.empty());

  AssociateExistingSyncIds();
  AssociateWindows(RELOAD_TABS, ScanForTabbedWindow(), initial_batch);
}

LocalSessionEventHandlerImpl::~LocalSessionEventHandlerImpl() {}

sync_pb::SessionTab
LocalSessionEventHandlerImpl::GetTabSpecificsFromDelegateForTest(
    const SyncedTabDelegate& tab_delegate) const {
  return GetTabSpecificsFromDelegate(tab_delegate);
}

void LocalSessionEventHandlerImpl::AssociateExistingSyncIds() {
  const SyncedWindowDelegatesGetter::SyncedWindowDelegateMap windows =
      sessions_client_->GetSyncedWindowDelegatesGetter()
          ->GetSyncedWindowDelegates();

  const std::map<int, std::vector<SyncedTabDelegate*>> tabs_per_sync_id =
      GroupTabsBySyncId(windows);

  for (const auto& sync_id_and_tabs : tabs_per_sync_id) {
    const int tab_node_id = sync_id_and_tabs.first;
    const std::vector<SyncedTabDelegate*>& tabs = sync_id_and_tabs.second;

    const SessionID initially_associated_tab_id =
        session_tracker_->LookupTabIdFromTabNodeId(current_session_tag_,
                                                   tab_node_id);
    const SyncedTabDelegate* selected_tab =
        tabs.size() == 1 ? tabs.front()
                         : ResolveConflictingSyncId(
                               tab_node_id, initially_associated_tab_id, tabs);
    // Execute the result of the conflict resolution.
    session_tracker_->ReassociateLocalTab(tab_node_id,
                                          selected_tab->GetSessionId());
    for (SyncedTabDelegate* synced_tab : tabs) {
      if (synced_tab != selected_tab) {
        synced_tab->SetSyncId(TabNodePool::kInvalidTabNodeID);
      }
    }
  }
}

void LocalSessionEventHandlerImpl::AssociateWindows(ReloadTabsOption option,
                                                    bool has_tabbed_window,
                                                    WriteBatch* batch) {
  // Note that |current_session| is a pointer owned by |session_tracker_|.
  // |session_tracker_| will continue to update |current_session| under
  // the hood so care must be taken accessing it. In particular, invoking
  // ResetSessionTracking(..) will invalidate all the tab data within
  // the session, hence why copies of the SyncedSession must be made ahead of
  // time.
  SyncedSession* current_session =
      session_tracker_->GetSession(current_session_tag_);

  SyncedWindowDelegatesGetter::SyncedWindowDelegateMap windows =
      sessions_client_->GetSyncedWindowDelegatesGetter()
          ->GetSyncedWindowDelegates();

  // Without native data, we need be careful not to obliterate any old
  // information, while at the same time handling updated tab ids. See
  // https://crbug.com/639009 for more info.
  if (has_tabbed_window) {
    // Just reset the session tracking. No need to worry about the previous
    // session; the current tabbed windows are now the source of truth.
    session_tracker_->ResetSessionTracking(current_session_tag_);
    current_session->modified_time = base::Time::Now();
  } else if (option == RELOAD_TABS) {
    DVLOG(1) << "Found no tabbed windows. Reloading "
             << current_session->windows.size()
             << " windows from previous session.";

    // A copy of the specifics must be made because |current_session| will be
    // updated in place and therefore can't be relied on as the source of truth.
    sync_pb::SessionSpecifics specifics;
    specifics.set_session_tag(current_session_tag_);
    current_session->ToSessionHeaderProto().Swap(specifics.mutable_header());
    UpdateTrackerWithSpecifics(specifics, base::Time::Now(), session_tracker_);

    // The tab entities stored in sync have outdated SessionId values. Go
    // through and update them to the new SessionIds.
    for (auto& win_iter : current_session->windows) {
      for (auto& tab : win_iter.second->wrapped_window.tabs) {
        int sync_id = session_tracker_->LookupTabNodeFromTabId(
            current_session_tag_, tab->tab_id);
        // In rare cases, the local model could contain a tab that doesn't
        // have a sync ID, if the restored header referenced a tab ID but the
        // tab entity didn't exist (e.g. was unsyncable).
        if (sync_id == TabNodePool::kInvalidTabNodeID) {
          continue;
        }

        DVLOG(1) << "Rewriting tab node " << sync_id << " with tab id "
                 << tab->tab_id.id();
        AppendChangeForExistingTab(sync_id, *tab, batch);
      }
    }
  }

  for (auto& window_iter_pair : windows) {
    const SyncedWindowDelegate* window_delegate = window_iter_pair.second;
    if (option == RELOAD_TABS) {
      UMA_HISTOGRAM_COUNTS("Sync.SessionTabs", window_delegate->GetTabCount());
    }

    // Make sure the window has tabs and a viewable window. The viewable
    // window check is necessary because, for example, when a browser is
    // closed the destructor is not necessarily run immediately. This means
    // its possible for us to get a handle to a browser that is about to be
    // removed. If the tab count is 0 or the window is null, the browser is
    // about to be deleted, so we ignore it.
    if (!IsWindowSyncable(*window_delegate)) {
      continue;
    }

    SessionID window_id = window_delegate->GetSessionId();
    DVLOG(1) << "Associating window " << window_id.id() << " with "
             << window_delegate->GetTabCount() << " tabs.";

    bool found_tabs = false;
    for (int j = 0; j < window_delegate->GetTabCount(); ++j) {
      SessionID tab_id = window_delegate->GetTabIdAt(j);
      SyncedTabDelegate* synced_tab = window_delegate->GetTabAt(j);

      // GetTabAt can return a null tab; in that case just skip it. Similarly,
      // if for some reason the tab id is invalid, skip it.
      if (!synced_tab || !tab_id.is_valid()) {
        continue;
      }

      // Placeholder tabs are those without WebContents, either because they
      // were never loaded into memory or they were evicted from memory
      // (typically only on Android devices). They only have a tab id,
      // window id, and a saved synced id (corresponding to the tab node
      // id). Note that only placeholders have this sync id, as it's
      // necessary to properly reassociate the tab with the entity that was
      // backing it.
      if (synced_tab->IsPlaceholderTab()) {
        // For tabs without WebContents update the |tab_id| and |window_id|,
        // as it could have changed after a session restore.
        if (synced_tab->GetSyncId() > TabNodePool::kInvalidTabNodeID) {
          AssociateRestoredPlaceholderTab(*synced_tab, tab_id, window_id,
                                          batch);
        } else {
          DVLOG(1) << "Placeholder tab " << tab_id.id() << " has no sync id.";
        }
      } else if (RELOAD_TABS == option) {
        AssociateTab(synced_tab, has_tabbed_window, batch);
      }

      // If the tab was syncable, it would have been added to the tracker
      // either by the above Associate[RestoredPlaceholder]Tab call or by
      // the OnLocalTabModified method invoking AssociateTab directly.
      // Therefore, we can key whether this window has valid tabs based on
      // the tab's presence in the tracker.
      const sessions::SessionTab* tab =
          session_tracker_->LookupSessionTab(current_session_tag_, tab_id);
      if (tab) {
        found_tabs = true;

        // Update this window's representation in the synced session tracker.
        // This is a no-op if called multiple times.
        session_tracker_->PutWindowInSession(current_session_tag_, window_id);

        // Put the tab in the window (must happen after the window is added
        // to the session).
        session_tracker_->PutTabInWindow(current_session_tag_, window_id,
                                         tab_id);
      }
    }
    if (found_tabs) {
      SyncedSessionWindow* synced_session_window =
          current_session->windows[window_id].get();
      if (window_delegate->IsTypeTabbed()) {
        synced_session_window->window_type =
            sync_pb::SessionWindow_BrowserType_TYPE_TABBED;
      } else if (window_delegate->IsTypePopup()) {
        synced_session_window->window_type =
            sync_pb::SessionWindow_BrowserType_TYPE_POPUP;
      } else {
        // This is a custom tab within an app. These will not be restored on
        // startup if not present.
        synced_session_window->window_type =
            sync_pb::SessionWindow_BrowserType_TYPE_CUSTOM_TAB;
      }
    }
  }

  std::set<int> deleted_tab_node_ids;
  session_tracker_->CleanupLocalTabs(&deleted_tab_node_ids);
  for (int tab_node_id : deleted_tab_node_ids) {
    batch->Delete(tab_node_id);
  }

  // Always update the header.  Sync takes care of dropping this update
  // if the entity specifics are identical (i.e windows, client name did
  // not change).
  auto specifics = std::make_unique<sync_pb::SessionSpecifics>();
  specifics->set_session_tag(current_session_tag_);
  current_session->ToSessionHeaderProto().Swap(specifics->mutable_header());
  batch->Put(std::move(specifics));
}

void LocalSessionEventHandlerImpl::AssociateTab(
    SyncedTabDelegate* const tab_delegate,
    bool has_tabbed_window,
    WriteBatch* batch) {
  DCHECK(!tab_delegate->IsPlaceholderTab());

  if (tab_delegate->IsBeingDestroyed()) {
    task_tracker_.CleanTabTasks(tab_delegate->GetSessionId());
    // Do nothing else. By not proactively adding the tab to the session, it
    // will be removed if necessary during subsequent cleanup.
    return;
  }

  // Ensure the task tracker has up to date task ids for this tab.
  UpdateTaskTracker(tab_delegate);

  if (!tab_delegate->ShouldSync(sessions_client_)) {
    return;
  }

  SessionID tab_id = tab_delegate->GetSessionId();
  DVLOG(1) << "Syncing tab " << tab_id.id() << " from window "
           << tab_delegate->GetWindowId().id();

  int tab_node_id =
      session_tracker_->LookupTabNodeFromTabId(current_session_tag_, tab_id);

  if (tab_node_id != TabNodePool::kInvalidTabNodeID) {
    DCHECK(tab_delegate->GetSyncId() == TabNodePool::kInvalidTabNodeID ||
           tab_delegate->GetSyncId() == tab_node_id);
  } else if (has_tabbed_window) {
    // Allocate a new (or reused) sync node for this tab.
    tab_node_id = session_tracker_->AssociateLocalTabWithFreeTabNode(tab_id);
    DCHECK_NE(TabNodePool::kInvalidTabNodeID, tab_node_id)
        << "https://crbug.com/639009";
  } else {
    // Only allowed to allocate sync ids when we have native data, which is only
    // true when we have a tabbed window. Without a sync id we cannot sync this
    // data, the tracker cannot even really track it. So don't do any more work.
    // This effectively breaks syncing custom tabs when the native browser isn't
    // fully loaded. Ideally this is fixed by saving tab data and sync data
    // atomically, see https://crbug.com/681921.
    return;
  }

  tab_delegate->SetSyncId(tab_node_id);

  // Get the previously synced url.
  sessions::SessionTab* session_tab =
      session_tracker_->GetTab(current_session_tag_, tab_id);
  int old_index = session_tab->normalized_navigation_index();
  GURL old_url;
  if (session_tab->navigations.size() > static_cast<size_t>(old_index))
    old_url = session_tab->navigations[old_index].virtual_url();

  // Produce the specifics.
  auto specifics = std::make_unique<sync_pb::SessionSpecifics>();
  specifics->set_session_tag(current_session_tag_);
  specifics->set_tab_node_id(tab_node_id);
  GetTabSpecificsFromDelegate(*tab_delegate).Swap(specifics->mutable_tab());
  WriteTasksIntoSpecifics(specifics->mutable_tab());

  // Update the tracker's session representation. Timestamp will be overwriten,
  // so we set a null time first to prevent the update from being ignored, if
  // the local clock is skewed.
  session_tab->timestamp = base::Time();
  UpdateTrackerWithSpecifics(*specifics, base::Time::Now(), session_tracker_);
  DCHECK(!session_tab->timestamp.is_null());

  // Write to the sync model itself.
  batch->Put(std::move(specifics));

  int current_index = tab_delegate->GetCurrentEntryIndex();
  const GURL new_url = tab_delegate->GetVirtualURLAtIndex(current_index);
  if (new_url != old_url) {
    delegate_->OnFaviconVisited(
        new_url, tab_delegate->GetFaviconURLAtIndex(current_index));
  }
}

void LocalSessionEventHandlerImpl::UpdateTaskTracker(
    SyncedTabDelegate* const tab_delegate) {
  TabTasks* tab_tasks = task_tracker_.GetTabTasks(
      tab_delegate->GetSessionId(), tab_delegate->GetSourceTabID());

  // Iterate through all navigations in the tab to ensure they all have a task
  // id set (it's possible some haven't been seen before, such as when a tab
  // is restored).
  for (int i = 0; i < tab_delegate->GetEntryCount(); ++i) {
    sessions::SerializedNavigationEntry serialized_entry;
    tab_delegate->GetSerializedNavigationAtIndex(i, &serialized_entry);

    int nav_id = serialized_entry.unique_id();
    int64_t global_id = serialized_entry.timestamp().ToInternalValue();
    tab_tasks->UpdateWithNavigation(
        nav_id, tab_delegate->GetTransitionAtIndex(i), global_id);
  }
}

void LocalSessionEventHandlerImpl::WriteTasksIntoSpecifics(
    sync_pb::SessionTab* tab_specifics) {
  TabTasks* tab_tasks = task_tracker_.GetTabTasks(
      SessionID::FromSerializedValue(tab_specifics->tab_id()),
      /*parent_tab_id=*/SessionID::InvalidValue());
  for (int i = 0; i < tab_specifics->navigation_size(); i++) {
    // Excluding blocked navigations, which are appended at tail.
    if (tab_specifics->navigation(i).blocked_state() ==
        sync_pb::TabNavigation::STATE_BLOCKED) {
      break;
    }

    std::vector<int64_t> task_ids = tab_tasks->GetTaskIdsForNavigation(
        tab_specifics->navigation(i).unique_id());
    if (task_ids.empty()) {
      continue;
    }

    tab_specifics->mutable_navigation(i)->set_task_id(task_ids.back());
    // Pop the task id of navigation self.
    task_ids.pop_back();
    tab_specifics->mutable_navigation(i)->clear_ancestor_task_id();
    for (auto ancestor_task_id : task_ids) {
      tab_specifics->mutable_navigation(i)->add_ancestor_task_id(
          ancestor_task_id);
    }
  }
}

void LocalSessionEventHandlerImpl::OnLocalTabModified(
    SyncedTabDelegate* modified_tab) {
  DCHECK(!current_session_tag_.empty());

  sessions::SerializedNavigationEntry current;
  modified_tab->GetSerializedNavigationAtIndex(
      modified_tab->GetCurrentEntryIndex(), &current);
  delegate_->TrackLocalNavigationId(current.timestamp(), current.unique_id());

  bool found_tabbed_window = ScanForTabbedWindow();
  std::unique_ptr<WriteBatch> batch = delegate_->CreateLocalSessionWriteBatch();
  AssociateExistingSyncIds();
  AssociateTab(modified_tab, found_tabbed_window, batch.get());
  // Note, we always associate windows because it's possible a tab became
  // "interesting" by going to a valid URL, in which case it needs to be added
  // to the window's tab information. Similarly, if a tab became
  // "uninteresting", we remove it from the window's tab information.
  AssociateWindows(DONT_RELOAD_TABS, found_tabbed_window, batch.get());
  batch->Commit();
}

void LocalSessionEventHandlerImpl::OnFaviconsChanged(
    const std::set<GURL>& page_urls,
    const GURL& /* icon_url */) {
  for (const GURL& page_url : page_urls) {
    if (page_url.is_valid()) {
      delegate_->OnPageFaviconUpdated(page_url);
    }
  }
}

void LocalSessionEventHandlerImpl::AssociateRestoredPlaceholderTab(
    const SyncedTabDelegate& tab_delegate,
    SessionID new_tab_id,
    SessionID new_window_id,
    WriteBatch* batch) {
  int tab_node_id = tab_delegate.GetSyncId();
  DCHECK_NE(tab_node_id, TabNodePool::kInvalidTabNodeID);
  DCHECK_EQ(new_tab_id, session_tracker_->LookupTabIdFromTabNodeId(
                            current_session_tag_, tab_node_id));

  // Update the window id on the SessionTab itself.
  sessions::SessionTab* local_tab =
      session_tracker_->GetTab(current_session_tag_, new_tab_id);
  local_tab->window_id = new_window_id;

  // Filter out placeholder tabs that have been associated but don't have known
  // sync data.
  if (local_tab->navigations.empty()) {
    return;
  }

  AppendChangeForExistingTab(tab_node_id, *local_tab, batch);
}

void LocalSessionEventHandlerImpl::AppendChangeForExistingTab(
    int sync_id,
    const sessions::SessionTab& tab,
    WriteBatch* batch) const {
  // Rewrite the specifics based on the reassociated SessionTab to preserve
  // the new tab and window ids.
  auto specifics = std::make_unique<sync_pb::SessionSpecifics>();
  tab.ToSyncData().Swap(specifics->mutable_tab());
  specifics->set_session_tag(current_session_tag_);
  specifics->set_tab_node_id(sync_id);
  batch->Put(std::move(specifics));
}

sync_pb::SessionTab LocalSessionEventHandlerImpl::GetTabSpecificsFromDelegate(
    const SyncedTabDelegate& tab_delegate) const {
  sync_pb::SessionTab specifics;
  specifics.set_window_id(tab_delegate.GetWindowId().id());
  specifics.set_tab_id(tab_delegate.GetSessionId().id());
  specifics.set_tab_visual_index(0);
  // Use -1 to indicate that the index hasn't been set properly yet.
  specifics.set_current_navigation_index(-1);
  const SyncedWindowDelegate* window_delegate =
      sessions_client_->GetSyncedWindowDelegatesGetter()->FindById(
          tab_delegate.GetWindowId());
  specifics.set_pinned(
      window_delegate ? window_delegate->IsTabPinned(&tab_delegate) : false);
  specifics.set_extension_app_id(tab_delegate.GetExtensionAppId());
  const int current_index = tab_delegate.GetCurrentEntryIndex();
  const int min_index = std::max(0, current_index - kMaxSyncNavigationCount);
  const int max_index = std::min(current_index + kMaxSyncNavigationCount,
                                 tab_delegate.GetEntryCount());
  bool is_supervised = tab_delegate.ProfileIsSupervised();

  for (int i = min_index; i < max_index; ++i) {
    if (!tab_delegate.GetVirtualURLAtIndex(i).is_valid()) {
      continue;
    }
    sessions::SerializedNavigationEntry serialized_entry;
    tab_delegate.GetSerializedNavigationAtIndex(i, &serialized_entry);

    // Set current_navigation_index to the index in navigations.
    if (i == current_index)
      specifics.set_current_navigation_index(specifics.navigation_size());

    sync_pb::TabNavigation* navigation = specifics.add_navigation();
    serialized_entry.ToSyncData().Swap(navigation);

    if (is_supervised) {
      navigation->set_blocked_state(
          sync_pb::TabNavigation_BlockedState_STATE_ALLOWED);
    }
  }

  // If the current navigation is invalid, set the index to the end of the
  // navigation array.
  if (specifics.current_navigation_index() < 0) {
    specifics.set_current_navigation_index(specifics.navigation_size() - 1);
  }

  if (is_supervised) {
    const std::vector<std::unique_ptr<const SerializedNavigationEntry>>&
        blocked_navigations = *tab_delegate.GetBlockedNavigations();
    for (size_t i = 0; i < blocked_navigations.size(); ++i) {
      sync_pb::TabNavigation* navigation = specifics.add_navigation();
      blocked_navigations[i]->ToSyncData().Swap(navigation);
      navigation->set_blocked_state(
          sync_pb::TabNavigation_BlockedState_STATE_BLOCKED);
      // TODO(bauerb): Add categories
    }
  }

  return specifics;
}

bool LocalSessionEventHandlerImpl::ScanForTabbedWindow() {
  for (const auto& window_iter_pair :
       sessions_client_->GetSyncedWindowDelegatesGetter()
           ->GetSyncedWindowDelegates()) {
    if (window_iter_pair.second->IsTypeTabbed()) {
      const SyncedWindowDelegate* window_delegate = window_iter_pair.second;
      if (IsWindowSyncable(*window_delegate)) {
        // When only custom tab windows are open, often we'll have a seemingly
        // okay type tabbed window, but GetTabAt will return null for each
        // index. This case is exactly what this method needs to protect
        // against.
        for (int j = 0; j < window_delegate->GetTabCount(); ++j) {
          if (window_delegate->GetTabAt(j)) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

}  // namespace sync_sessions
