// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SESSIONS_TEST_SYNCED_WINDOW_DELEGATES_GETTER_H_
#define COMPONENTS_SYNC_SESSIONS_TEST_SYNCED_WINDOW_DELEGATES_GETTER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/time/time.h"
#include "components/sync/protocol/session_specifics.pb.h"
#include "components/sync_sessions/local_session_event_router.h"
#include "components/sync_sessions/synced_tab_delegate.h"
#include "components/sync_sessions/synced_window_delegate.h"
#include "components/sync_sessions/synced_window_delegates_getter.h"

namespace sync_sessions {

// A SyncedTabDelegate fake for testing. It simulates a normal
// SyncedTabDelegate with a proper WebContents. For a SyncedTabDelegate without
// a WebContents, see PlaceholderTabDelegate below.
class TestSyncedTabDelegate : public SyncedTabDelegate {
 public:
  TestSyncedTabDelegate(
      SessionID window_id,
      SessionID tab_id,
      const base::RepeatingCallback<void(SyncedTabDelegate*)>& notify_cb);
  ~TestSyncedTabDelegate() override;

  void Navigate(const std::string& url,
                base::Time time = base::Time::Now(),
                ui::PageTransition transition = ui::PAGE_TRANSITION_TYPED);
  void set_current_entry_index(int i);
  void set_blocked_navigations(
      const std::vector<std::unique_ptr<sessions::SerializedNavigationEntry>>&
          navs);

  // SyncedTabDelegate overrides.
  bool IsInitialBlankNavigation() const override;
  int GetCurrentEntryIndex() const override;
  GURL GetVirtualURLAtIndex(int i) const override;
  GURL GetFaviconURLAtIndex(int i) const override;
  ui::PageTransition GetTransitionAtIndex(int i) const override;
  void GetSerializedNavigationAtIndex(
      int i,
      sessions::SerializedNavigationEntry* serialized_entry) const override;
  int GetEntryCount() const override;
  SessionID GetWindowId() const override;
  SessionID GetSessionId() const override;
  bool IsBeingDestroyed() const override;
  std::string GetExtensionAppId() const override;
  bool ProfileIsSupervised() const override;
  void set_is_supervised(bool is_supervised);
  const std::vector<std::unique_ptr<const sessions::SerializedNavigationEntry>>*
  GetBlockedNavigations() const override;
  bool IsPlaceholderTab() const override;
  int GetSyncId() const override;
  void SetSyncId(int sync_id) override;
  bool ShouldSync(SyncSessionsClient* sessions_client) override;
  SessionID GetSourceTabID() const override;

 private:
  const SessionID window_id_;
  const SessionID tab_id_;
  const base::RepeatingCallback<void(SyncedTabDelegate*)> notify_cb_;

  int current_entry_index_ = -1;
  bool is_supervised_ = false;
  int sync_id_ = -1;
  std::vector<std::unique_ptr<const sessions::SerializedNavigationEntry>>
      blocked_navigations_;
  std::vector<std::unique_ptr<const sessions::SerializedNavigationEntry>>
      entries_;

  DISALLOW_COPY_AND_ASSIGN(TestSyncedTabDelegate);
};

// A placeholder delegate. These delegates have no WebContents, simulating a tab
// that has been restored without bringing its state fully into memory (for
// example on Android), or where the tab's contents have been evicted from
// memory. See SyncedTabDelegate::IsPlaceHolderTab for more info.
class PlaceholderTabDelegate : public SyncedTabDelegate {
 public:
  PlaceholderTabDelegate(SessionID tab_id, int sync_id);
  ~PlaceholderTabDelegate() override;

  // SyncedTabDelegate overrides.
  SessionID GetSessionId() const override;
  int GetSyncId() const override;
  void SetSyncId(int sync_id) override;
  bool IsPlaceholderTab() const override;
  // Everything else is invalid to invoke as it depends on a valid WebContents.
  SessionID GetWindowId() const override;
  bool IsBeingDestroyed() const override;
  std::string GetExtensionAppId() const override;
  bool IsInitialBlankNavigation() const override;
  int GetCurrentEntryIndex() const override;
  int GetEntryCount() const override;
  GURL GetVirtualURLAtIndex(int i) const override;
  GURL GetFaviconURLAtIndex(int i) const override;
  ui::PageTransition GetTransitionAtIndex(int i) const override;
  void GetSerializedNavigationAtIndex(
      int i,
      sessions::SerializedNavigationEntry* serialized_entry) const override;
  bool ProfileIsSupervised() const override;
  const std::vector<std::unique_ptr<const sessions::SerializedNavigationEntry>>*
  GetBlockedNavigations() const override;
  bool ShouldSync(SyncSessionsClient* sessions_client) override;
  SessionID GetSourceTabID() const override;

 private:
  const SessionID tab_id_;
  int sync_id_ = -1;

  DISALLOW_COPY_AND_ASSIGN(PlaceholderTabDelegate);
};

class TestSyncedWindowDelegate : public SyncedWindowDelegate {
 public:
  explicit TestSyncedWindowDelegate(SessionID window_id,
                                    sync_pb::SessionWindow_BrowserType type);
  ~TestSyncedWindowDelegate() override;

  // |delegate| must not be nullptr and must outlive this object.
  void OverrideTabAt(int index, SyncedTabDelegate* delegate);

  void SetIsSessionRestoreInProgress(bool value);

  // SyncedWindowDelegate overrides.
  bool HasWindow() const override;
  SessionID GetSessionId() const override;
  int GetTabCount() const override;
  int GetActiveIndex() const override;
  bool IsApp() const override;
  bool IsTypeTabbed() const override;
  bool IsTypePopup() const override;
  bool IsTabPinned(const SyncedTabDelegate* tab) const override;
  SyncedTabDelegate* GetTabAt(int index) const override;
  SessionID GetTabIdAt(int index) const override;
  bool IsSessionRestoreInProgress() const override;
  bool ShouldSync() const override;

 private:
  const SessionID window_id_;
  const sync_pb::SessionWindow_BrowserType window_type_;

  std::map<int, SyncedTabDelegate*> tab_delegates_;
  std::map<int, SyncedTabDelegate*> tab_overrides_;
  std::map<int, SessionID> tab_id_overrides_;
  bool is_session_restore_in_progress_;

  DISALLOW_COPY_AND_ASSIGN(TestSyncedWindowDelegate);
};

class TestSyncedWindowDelegatesGetter : public SyncedWindowDelegatesGetter {
 public:
  TestSyncedWindowDelegatesGetter();
  ~TestSyncedWindowDelegatesGetter() override;

  void ResetWindows();
  TestSyncedWindowDelegate* AddWindow(
      sync_pb::SessionWindow_BrowserType type,
      SessionID window_id = SessionID::NewUnique());
  // Creates a new tab within the window specified by |window_id|. The newly
  // created tab's ID can be specified optionally. Returns the newly created
  // TestSyncedTabDelegate (not owned).
  TestSyncedTabDelegate* AddTab(SessionID window_id,
                                SessionID tab_id = SessionID::NewUnique());
  LocalSessionEventRouter* router();

  // SyncedWindowDelegatesGetter overrides.
  SyncedWindowDelegateMap GetSyncedWindowDelegates() override;
  const SyncedWindowDelegate* FindById(SessionID id) override;

 private:
  class DummyRouter : public LocalSessionEventRouter {
   public:
    DummyRouter();
    ~DummyRouter() override;
    void StartRoutingTo(LocalSessionEventHandler* handler) override;
    void Stop() override;
    void NotifyNav(SyncedTabDelegate* tab);

   private:
    LocalSessionEventHandler* handler_ = nullptr;
  };

  SyncedWindowDelegateMap delegates_;
  std::vector<std::unique_ptr<TestSyncedWindowDelegate>> windows_;
  std::vector<std::unique_ptr<TestSyncedTabDelegate>> tabs_;
  DummyRouter router_;

  DISALLOW_COPY_AND_ASSIGN(TestSyncedWindowDelegatesGetter);
};

}  // namespace sync_sessions

#endif  // COMPONENTS_SYNC_SESSIONS_TEST_SYNCED_WINDOW_DELEGATES_GETTER_H_
