// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/sessions/tab_delegate_sync_adapter.h"

#include <string>

#include "base/macros.h"
#include "components/sync/driver/fake_sync_service.h"
#include "components/sync_sessions/open_tabs_ui_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::RefCountedMemory;
using sessions::SessionTab;
using sessions::SessionWindow;
using syncer::SyncServiceObserver;
using sync_sessions::SyncedSession;
using sync_sessions::OpenTabsUIDelegate;
using testing::Test;

namespace ntp_snippets {
namespace {

class TestSyncService : public syncer::FakeSyncService {
 public:
  TestSyncService() {}
  OpenTabsUIDelegate* GetOpenTabsUIDelegate() override { return tabs_; }
  OpenTabsUIDelegate* tabs_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestSyncService);
};

class MockOpenTabsUIDelegate : public OpenTabsUIDelegate {
 public:
  MockOpenTabsUIDelegate() {}
  MOCK_CONST_METHOD2(GetSyncedFaviconForPageURL,
                     bool(const std::string&,
                          scoped_refptr<RefCountedMemory>*));
  MOCK_METHOD1(GetAllForeignSessions, bool(std::vector<const SyncedSession*>*));
  MOCK_METHOD3(GetForeignTab,
               bool(const std::string&, SessionID, const SessionTab**));
  MOCK_METHOD1(DeleteForeignSession, void(const std::string&));
  MOCK_METHOD2(GetForeignSession,
               bool(const std::string&, std::vector<const SessionWindow*>*));
  MOCK_METHOD2(GetForeignSessionTabs,
               bool(const std::string&, std::vector<const SessionTab*>*));
  MOCK_METHOD1(GetLocalSession, bool(const SyncedSession**));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockOpenTabsUIDelegate);
};

class TabDelegateSyncAdapterTest : public Test {
 public:
  TabDelegateSyncAdapterTest() : adapter_(service()) {
    adapter_.SubscribeForForeignTabChange(base::Bind(
        &TabDelegateSyncAdapterTest::OnChange, base::Unretained(this)));
  }

  syncer::SyncService* service() { return &service_; }
  SyncServiceObserver* observer() { return &adapter_; }

  void SetHasOpenTabs(bool is_enabled) {
    service_.tabs_ = is_enabled ? &tabs_ : nullptr;
  }

  void OnChange() { ++callback_count_; }

  int CallbackCount() { return callback_count_; }

 private:
  MockOpenTabsUIDelegate tabs_;
  TestSyncService service_;
  TabDelegateSyncAdapter adapter_;
  int callback_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TabDelegateSyncAdapterTest);
};

// CallbackCount should only trigger on transitions between having and not
// having open tabs.
TEST_F(TabDelegateSyncAdapterTest, CallbackCount) {
  ASSERT_EQ(0, CallbackCount());
  observer()->OnStateChanged(service());
  EXPECT_EQ(0, CallbackCount());

  SetHasOpenTabs(true);
  observer()->OnStateChanged(service());
  EXPECT_EQ(1, CallbackCount());
  observer()->OnStateChanged(service());
  EXPECT_EQ(1, CallbackCount());

  SetHasOpenTabs(false);
  observer()->OnStateChanged(service());
  EXPECT_EQ(2, CallbackCount());
  observer()->OnStateChanged(service());
  EXPECT_EQ(2, CallbackCount());

  // OnSyncCycleCompleted should behave like OnStateChanged.
  observer()->OnSyncCycleCompleted(service());
  EXPECT_EQ(2, CallbackCount());
  SetHasOpenTabs(true);
  observer()->OnSyncCycleCompleted(service());
  EXPECT_EQ(3, CallbackCount());
  observer()->OnSyncCycleCompleted(service());
  EXPECT_EQ(3, CallbackCount());
}

// No callback should be invoked from OnSyncConfigurationCompleted.
TEST_F(TabDelegateSyncAdapterTest, OnSyncConfigurationCompleted) {
  ASSERT_EQ(0, CallbackCount());

  observer()->OnSyncConfigurationCompleted(service());
  EXPECT_EQ(0, CallbackCount());

  SetHasOpenTabs(true);
  observer()->OnSyncConfigurationCompleted(service());
  EXPECT_EQ(0, CallbackCount());
}

// OnForeignSessionUpdated should always trigger a callback.
TEST_F(TabDelegateSyncAdapterTest, OnForeignSessionUpdated) {
  ASSERT_EQ(0, CallbackCount());
  observer()->OnForeignSessionUpdated(service());
  EXPECT_EQ(1, CallbackCount());
  observer()->OnForeignSessionUpdated(service());
  EXPECT_EQ(2, CallbackCount());

  SetHasOpenTabs(true);
  observer()->OnForeignSessionUpdated(service());
  EXPECT_EQ(3, CallbackCount());
  observer()->OnForeignSessionUpdated(service());
  EXPECT_EQ(4, CallbackCount());
}

// If OnForeignSessionUpdated is called before OnStateChanged, then calling
// OnStateChanged should not trigger a callback.
TEST_F(TabDelegateSyncAdapterTest, OnForeignSessionUpdatedUpdatesState) {
  SetHasOpenTabs(true);
  observer()->OnForeignSessionUpdated(service());
  EXPECT_EQ(1, CallbackCount());

  observer()->OnStateChanged(service());
  EXPECT_EQ(1, CallbackCount());
}

}  // namespace
}  // namespace ntp_snippets
