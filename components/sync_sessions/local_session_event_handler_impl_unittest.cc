// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_sessions/local_session_event_handler_impl.h"

#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "components/sessions/core/serialized_navigation_entry_test_helper.h"
#include "components/sync/base/time.h"
#include "components/sync/model/sync_change.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync_sessions/mock_sync_sessions_client.h"
#include "components/sync_sessions/synced_session_tracker.h"
#include "components/sync_sessions/test_matchers.h"
#include "components/sync_sessions/test_synced_window_delegates_getter.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sync_sessions {
namespace {

using sessions::SerializedNavigationEntry;
using sessions::SerializedNavigationEntryTestHelper;
using testing::ByMove;
using testing::Eq;
using testing::IsEmpty;
using testing::NiceMock;
using testing::Pointee;
using testing::Return;
using testing::SizeIs;
using testing::StrictMock;
using testing::_;

const char kFoo1[] = "http://foo1/";
const char kBar1[] = "http://bar1/";
const char kBar2[] = "http://bar2/";
const char kBaz1[] = "http://baz1/";

const char kSessionTag[] = "sessiontag1";
const char kSessionName[] = "Session Name 1";

const base::Time kTime0 = base::Time::FromInternalValue(100);
const base::Time kTime1 = base::Time::FromInternalValue(110);
const base::Time kTime2 = base::Time::FromInternalValue(120);
const base::Time kTime3 = base::Time::FromInternalValue(130);

const int kWindowId1 = 1000001;
const int kWindowId2 = 1000002;
const int kWindowId3 = 1000003;
const int kTabId1 = 1000004;
const int kTabId2 = 1000005;
const int kTabId3 = 1000006;

class MockWriteBatch : public LocalSessionEventHandlerImpl::WriteBatch {
 public:
  MockWriteBatch() {}
  ~MockWriteBatch() override {}

  MOCK_METHOD1(Delete, void(int tab_node_id));
  MOCK_METHOD1(Put, void(std::unique_ptr<sync_pb::SessionSpecifics> specifics));
  MOCK_METHOD0(Commit, void());
};

class MockDelegate : public LocalSessionEventHandlerImpl::Delegate {
 public:
  ~MockDelegate() override {}

  MOCK_METHOD0(CreateLocalSessionWriteBatch,
               std::unique_ptr<LocalSessionEventHandlerImpl::WriteBatch>());
  MOCK_METHOD2(TrackLocalNavigationId,
               void(base::Time timestamp, int unique_id));
  MOCK_METHOD1(OnPageFaviconUpdated, void(const GURL& page_url));
  MOCK_METHOD2(OnFaviconVisited,
               void(const GURL& page_url, const GURL& favicon_url));
};

class LocalSessionEventHandlerImplTest : public testing::Test {
 public:
  LocalSessionEventHandlerImplTest()
      : session_tracker_(&mock_sync_sessions_client_) {
    ON_CALL(mock_sync_sessions_client_, GetSyncedWindowDelegatesGetter())
        .WillByDefault(testing::Return(&window_getter_));

    session_tracker_.InitLocalSession(kSessionTag, kSessionName,
                                      sync_pb::SyncEnums_DeviceType_TYPE_PHONE);
  }

  void InitHandler(LocalSessionEventHandlerImpl::WriteBatch* initial_batch) {
    handler_ = std::make_unique<LocalSessionEventHandlerImpl>(
        &mock_delegate_, &mock_sync_sessions_client_, &session_tracker_,
        initial_batch);
    window_getter_.router()->StartRoutingTo(handler_.get());
  }

  void InitHandler() {
    NiceMock<MockWriteBatch> initial_batch;
    InitHandler(&initial_batch);
  }

  TestSyncedWindowDelegate* AddWindow(
      int window_id,
      sync_pb::SessionWindow_BrowserType type =
          sync_pb::SessionWindow_BrowserType_TYPE_TABBED) {
    return window_getter_.AddWindow(type,
                                    SessionID::FromSerializedValue(window_id));
  }

  TestSyncedTabDelegate* AddTab(int window_id,
                                const std::string& url,
                                int tab_id = SessionID::NewUnique().id()) {
    TestSyncedTabDelegate* tab =
        window_getter_.AddTab(SessionID::FromSerializedValue(window_id),
                              SessionID::FromSerializedValue(tab_id));
    tab->Navigate(url, base::Time::Now());
    return tab;
  }

  TestSyncedTabDelegate* AddTabWithTime(int window_id,
                                        const std::string& url,
                                        base::Time time = base::Time::Now()) {
    TestSyncedTabDelegate* tab =
        window_getter_.AddTab(SessionID::FromSerializedValue(window_id));
    tab->Navigate(url, time);
    return tab;
  }

  testing::NiceMock<MockDelegate> mock_delegate_;
  testing::NiceMock<MockSyncSessionsClient> mock_sync_sessions_client_;
  SyncedSessionTracker session_tracker_;
  TestSyncedWindowDelegatesGetter window_getter_;
  std::unique_ptr<LocalSessionEventHandlerImpl> handler_;
};

// Populate the mock tab delegate with some data and navigation
// entries and make sure that populating a SessionTab contains analgous
// information.
TEST_F(LocalSessionEventHandlerImplTest, GetTabSpecificsFromDelegate) {
  // Create a tab with three valid entries.
  AddWindow(kWindowId1);
  TestSyncedTabDelegate* tab = AddTabWithTime(kWindowId1, kFoo1, kTime1);
  tab->Navigate(kBar1, kTime2);
  tab->Navigate(kBaz1, kTime3);
  InitHandler();

  const sync_pb::SessionTab session_tab =
      handler_->GetTabSpecificsFromDelegateForTest(*tab);

  EXPECT_EQ(tab->GetWindowId().id(), session_tab.window_id());
  EXPECT_EQ(tab->GetSessionId().id(), session_tab.tab_id());
  EXPECT_EQ(0, session_tab.tab_visual_index());
  EXPECT_EQ(tab->GetCurrentEntryIndex(),
            session_tab.current_navigation_index());
  EXPECT_FALSE(session_tab.pinned());
  EXPECT_TRUE(session_tab.extension_app_id().empty());
  ASSERT_EQ(3, session_tab.navigation_size());
  EXPECT_EQ(GURL(kFoo1), session_tab.navigation(0).virtual_url());
  EXPECT_EQ(GURL(kBar1), session_tab.navigation(1).virtual_url());
  EXPECT_EQ(GURL(kBaz1), session_tab.navigation(2).virtual_url());
  EXPECT_EQ(syncer::TimeToProtoTime(kTime1),
            session_tab.navigation(0).timestamp_msec());
  EXPECT_EQ(syncer::TimeToProtoTime(kTime2),
            session_tab.navigation(1).timestamp_msec());
  EXPECT_EQ(syncer::TimeToProtoTime(kTime3),
            session_tab.navigation(2).timestamp_msec());
  EXPECT_EQ(200, session_tab.navigation(0).http_status_code());
  EXPECT_EQ(200, session_tab.navigation(1).http_status_code());
  EXPECT_EQ(200, session_tab.navigation(2).http_status_code());
  EXPECT_FALSE(session_tab.navigation(0).has_blocked_state());
  EXPECT_FALSE(session_tab.navigation(1).has_blocked_state());
  EXPECT_FALSE(session_tab.navigation(2).has_blocked_state());
}

// Ensure the current_navigation_index gets set properly when the navigation
// stack gets trucated to +/- 6 entries.
TEST_F(LocalSessionEventHandlerImplTest,
       SetSessionTabFromDelegateNavigationIndex) {
  AddWindow(kWindowId1);
  TestSyncedTabDelegate* tab = AddTab(kWindowId1, kFoo1);
  const int kNavs = 10;
  for (int i = 1; i < kNavs; ++i) {
    tab->Navigate(base::StringPrintf("http://foo%i", i));
  }
  tab->set_current_entry_index(kNavs - 2);

  InitHandler();

  const sync_pb::SessionTab session_tab =
      handler_->GetTabSpecificsFromDelegateForTest(*tab);

  EXPECT_EQ(6, session_tab.current_navigation_index());
  ASSERT_EQ(8, session_tab.navigation_size());
  EXPECT_EQ(GURL("http://foo2"), session_tab.navigation(0).virtual_url());
  EXPECT_EQ(GURL("http://foo3"), session_tab.navigation(1).virtual_url());
  EXPECT_EQ(GURL("http://foo4"), session_tab.navigation(2).virtual_url());
}

// Ensure the current_navigation_index gets set to the end of the navigation
// stack if the current navigation is invalid.
TEST_F(LocalSessionEventHandlerImplTest,
       SetSessionTabFromDelegateCurrentInvalid) {
  AddWindow(kWindowId1);
  TestSyncedTabDelegate* tab = AddTabWithTime(kWindowId1, kFoo1, kTime0);
  tab->Navigate(std::string(""), kTime1);
  tab->Navigate(kBar1, kTime2);
  tab->Navigate(kBar2, kTime3);
  tab->set_current_entry_index(1);

  InitHandler();

  const sync_pb::SessionTab session_tab =
      handler_->GetTabSpecificsFromDelegateForTest(*tab);

  EXPECT_EQ(2, session_tab.current_navigation_index());
  ASSERT_EQ(3, session_tab.navigation_size());
}

// Tests that for supervised users blocked navigations are recorded and marked
// as such, while regular navigations are marked as allowed.
TEST_F(LocalSessionEventHandlerImplTest, BlockedNavigations) {
  AddWindow(kWindowId1);
  TestSyncedTabDelegate* tab = AddTabWithTime(kWindowId1, kFoo1, kTime1);

  auto entry2 = std::make_unique<sessions::SerializedNavigationEntry>();
  GURL url2("http://blocked.com/foo");
  SerializedNavigationEntryTestHelper::SetVirtualURL(GURL(url2), entry2.get());
  SerializedNavigationEntryTestHelper::SetTimestamp(kTime2, entry2.get());

  auto entry3 = std::make_unique<sessions::SerializedNavigationEntry>();
  GURL url3("http://evil.com");
  SerializedNavigationEntryTestHelper::SetVirtualURL(GURL(url3), entry3.get());
  SerializedNavigationEntryTestHelper::SetTimestamp(kTime3, entry3.get());

  std::vector<std::unique_ptr<sessions::SerializedNavigationEntry>>
      blocked_navigations;
  blocked_navigations.push_back(std::move(entry2));
  blocked_navigations.push_back(std::move(entry3));

  tab->set_is_supervised(true);
  tab->set_blocked_navigations(blocked_navigations);

  InitHandler();
  const sync_pb::SessionTab session_tab =
      handler_->GetTabSpecificsFromDelegateForTest(*tab);

  EXPECT_EQ(tab->GetWindowId().id(), session_tab.window_id());
  EXPECT_EQ(tab->GetSessionId().id(), session_tab.tab_id());
  EXPECT_EQ(0, session_tab.tab_visual_index());
  EXPECT_EQ(0, session_tab.current_navigation_index());
  EXPECT_FALSE(session_tab.pinned());
  ASSERT_EQ(3, session_tab.navigation_size());
  EXPECT_EQ(GURL(kFoo1), session_tab.navigation(0).virtual_url());
  EXPECT_EQ(url2, session_tab.navigation(1).virtual_url());
  EXPECT_EQ(url3, session_tab.navigation(2).virtual_url());
  EXPECT_EQ(syncer::TimeToProtoTime(kTime1),
            session_tab.navigation(0).timestamp_msec());
  EXPECT_EQ(syncer::TimeToProtoTime(kTime2),
            session_tab.navigation(1).timestamp_msec());
  EXPECT_EQ(syncer::TimeToProtoTime(kTime3),
            session_tab.navigation(2).timestamp_msec());
  EXPECT_TRUE(session_tab.navigation(0).has_blocked_state());
  EXPECT_TRUE(session_tab.navigation(1).has_blocked_state());
  EXPECT_TRUE(session_tab.navigation(2).has_blocked_state());
  EXPECT_EQ(sync_pb::TabNavigation_BlockedState_STATE_ALLOWED,
            session_tab.navigation(0).blocked_state());
  EXPECT_EQ(sync_pb::TabNavigation_BlockedState_STATE_BLOCKED,
            session_tab.navigation(1).blocked_state());
  EXPECT_EQ(sync_pb::TabNavigation_BlockedState_STATE_BLOCKED,
            session_tab.navigation(2).blocked_state());
}

// Tests that calling AssociateWindowsAndTabs() handles well the case with no
// open tabs or windows.
TEST_F(LocalSessionEventHandlerImplTest, AssociateWindowsAndTabsIfEmpty) {
  EXPECT_CALL(mock_delegate_, CreateLocalSessionWriteBatch()).Times(0);
  EXPECT_CALL(mock_delegate_, OnPageFaviconUpdated(_)).Times(0);
  EXPECT_CALL(mock_delegate_, OnFaviconVisited(_, _)).Times(0);

  StrictMock<MockWriteBatch> mock_batch;
  EXPECT_CALL(mock_batch,
              Put(Pointee(MatchesHeader(kSessionTag, /*window_ids=*/IsEmpty(),
                                        /*tabs_ids=*/IsEmpty()))));

  InitHandler(&mock_batch);
}

// Tests that calling AssociateWindowsAndTabs() reflects the open tabs in a) the
// SyncSessionTracker and b) the delegate.
TEST_F(LocalSessionEventHandlerImplTest, AssociateWindowsAndTabs) {
  AddWindow(kWindowId1);
  AddTab(kWindowId1, kFoo1, kTabId1);
  AddWindow(kWindowId2);
  AddTab(kWindowId2, kBar1, kTabId2);
  AddTab(kWindowId2, kBar2, kTabId3)->Navigate(kBaz1);

  EXPECT_CALL(mock_delegate_, CreateLocalSessionWriteBatch()).Times(0);
  EXPECT_CALL(mock_delegate_, OnPageFaviconUpdated(_)).Times(0);
  EXPECT_CALL(mock_delegate_, OnFaviconVisited(GURL(kBar2), _)).Times(0);

  EXPECT_CALL(mock_delegate_, OnFaviconVisited(GURL(kFoo1), _));
  EXPECT_CALL(mock_delegate_, OnFaviconVisited(GURL(kBar1), _));
  EXPECT_CALL(mock_delegate_, OnFaviconVisited(GURL(kBaz1), _));

  StrictMock<MockWriteBatch> mock_batch;
  EXPECT_CALL(mock_batch,
              Put(Pointee(MatchesHeader(kSessionTag, {kWindowId1, kWindowId2},
                                        {kTabId1, kTabId2, kTabId3}))));
  EXPECT_CALL(mock_batch,
              Put(Pointee(MatchesTab(kSessionTag, kWindowId1, kTabId1,
                                     /*tab_node_id=*/_,
                                     /*urls=*/{kFoo1}))));
  EXPECT_CALL(mock_batch,
              Put(Pointee(MatchesTab(kSessionTag, kWindowId2, kTabId2,
                                     /*tab_node_id=*/_, /*urls=*/{kBar1}))));
  EXPECT_CALL(
      mock_batch,
      Put(Pointee(MatchesTab(kSessionTag, kWindowId2, kTabId3,
                             /*tab_node_id=*/_, /*urls=*/{kBar2, kBaz1}))));

  InitHandler(&mock_batch);
}

// Tests that calling AssociateWindowsAndTabs() reflects the open tabs in a) the
// SyncSessionTracker and b) the delegate, for the case where a custom tab
// exists without native data (no tabbed window).
TEST_F(LocalSessionEventHandlerImplTest, AssociateCustomTab) {
  const int kRegularTabNodeId = 1;
  const int kCustomTabNodeId = 2;

  // The tracker is initially restored from persisted state, containing a
  // regular tab and a custom tab. This mimics
  // SessionsSyncManager::InitFromSyncModel().
  sync_pb::SessionSpecifics regular_tab;
  regular_tab.set_session_tag(kSessionTag);
  regular_tab.set_tab_node_id(kRegularTabNodeId);
  regular_tab.mutable_tab()->set_window_id(kWindowId1);
  regular_tab.mutable_tab()->set_tab_id(kTabId1);
  session_tracker_.ReassociateLocalTab(kRegularTabNodeId,
                                       SessionID::FromSerializedValue(kTabId1));
  UpdateTrackerWithSpecifics(regular_tab, base::Time::Now(), &session_tracker_);

  sync_pb::SessionSpecifics custom_tab;
  custom_tab.set_session_tag(kSessionTag);
  custom_tab.set_tab_node_id(kCustomTabNodeId);
  custom_tab.mutable_tab()->set_window_id(kWindowId2);
  custom_tab.mutable_tab()->set_tab_id(kTabId2);
  session_tracker_.ReassociateLocalTab(kCustomTabNodeId,
                                       SessionID::FromSerializedValue(kTabId2));
  UpdateTrackerWithSpecifics(custom_tab, base::Time::Now(), &session_tracker_);

  sync_pb::SessionSpecifics header;
  header.set_session_tag(kSessionTag);
  header.mutable_header()->add_window()->set_window_id(kWindowId1);
  header.mutable_header()->mutable_window(0)->add_tab(kTabId1);
  header.mutable_header()->add_window()->set_window_id(kWindowId2);
  header.mutable_header()->mutable_window(1)->add_tab(kTabId2);
  UpdateTrackerWithSpecifics(header, base::Time::Now(), &session_tracker_);

  ASSERT_THAT(session_tracker_.LookupSession(kSessionTag),
              MatchesSyncedSession(kSessionTag,
                                   {{kWindowId1, std::vector<int>{kTabId1}},
                                    {kWindowId2, std::vector<int>{kTabId2}}}));

  // In the current session, all we have is a custom tab.
  AddWindow(kWindowId3, sync_pb::SessionWindow_BrowserType_TYPE_CUSTOM_TAB);
  AddTab(kWindowId3, kFoo1, kTabId3)->SetSyncId(kCustomTabNodeId);

  EXPECT_CALL(mock_delegate_, CreateLocalSessionWriteBatch()).Times(0);

  StrictMock<MockWriteBatch> mock_batch;
  testing::InSequence seq;
  EXPECT_CALL(mock_batch,
              Put(Pointee(MatchesTab(kSessionTag, kWindowId1, kTabId1,
                                     kRegularTabNodeId, /*urls=*/{}))));
  // Overriden by the Put() below, so we don't care about the args.
  EXPECT_CALL(mock_batch,
              Put(Pointee(MatchesTab(kSessionTag, _, _, kCustomTabNodeId,
                                     /*urls=*/_))));
  EXPECT_CALL(mock_batch, Put(Pointee(MatchesTab(kSessionTag, kWindowId3,
                                                 kTabId3, kCustomTabNodeId,
                                                 /*urls=*/{kFoo1}))));
  EXPECT_CALL(mock_batch, Put(Pointee(MatchesHeader(
                              kSessionTag, {kWindowId1, kWindowId2, kWindowId3},
                              {kTabId1, kTabId3}))));
  InitHandler(&mock_batch);

  EXPECT_THAT(session_tracker_.LookupSession(kSessionTag),
              MatchesSyncedSession(kSessionTag,
                                   {{kWindowId1, std::vector<int>{kTabId1}},
                                    {kWindowId2, std::vector<int>()},
                                    {kWindowId3, std::vector<int>{kTabId3}}}));
}

// Tests that calling initial association during construction handles the case
// where only a subset of tabs (and not the first) have a sync ID.
TEST_F(LocalSessionEventHandlerImplTest, AssociateTabsWhenOnlySomeHaveNodeIds) {
  const int kTabNodeId = 0;

  AddWindow(kWindowId1);
  AddTab(kWindowId1, kFoo1, kTabId1);
  AddTab(kWindowId1, kBar1, kTabId2)->SetSyncId(kTabNodeId);

  StrictMock<MockWriteBatch> mock_batch;
  EXPECT_CALL(mock_batch, Put(Pointee(MatchesHeader(_, _, _))));
  EXPECT_CALL(mock_batch,
              Put(Pointee(MatchesTab(_, _, kTabId1, /*tab_node_id=*/1,
                                     /*urls=*/_))));
  EXPECT_CALL(mock_batch,
              Put(Pointee(MatchesTab(_, _, kTabId2,
                                     /*tab_node_id=*/kTabNodeId, /*urls=*/_))));

  InitHandler(&mock_batch);
}

TEST_F(LocalSessionEventHandlerImplTest, PropagateNewNavigation) {
  AddWindow(kWindowId1);
  TestSyncedTabDelegate* tab = AddTab(kWindowId1, kFoo1, kTabId1);

  InitHandler();

  auto update_mock_batch = std::make_unique<StrictMock<MockWriteBatch>>();
  // Note that the header is reported again, although it hasn't changed. This is
  // OK because sync will avoid updating an entity with identical content.
  EXPECT_CALL(
      *update_mock_batch,
      Put(Pointee(MatchesHeader(kSessionTag, {kWindowId1}, {kTabId1}))));
  EXPECT_CALL(*update_mock_batch,
              Put(Pointee(MatchesTab(kSessionTag, kWindowId1, kTabId1,
                                     /*tab_node_id=*/_,
                                     /*urls=*/{kFoo1, kBar1}))));
  EXPECT_CALL(*update_mock_batch, Commit());

  EXPECT_CALL(mock_delegate_, CreateLocalSessionWriteBatch())
      .WillOnce(Return(ByMove(std::move(update_mock_batch))));

  tab->Navigate(kBar1);
}

TEST_F(LocalSessionEventHandlerImplTest, PropagateNewTab) {
  AddWindow(kWindowId1);
  AddTab(kWindowId1, kFoo1, kTabId1);

  InitHandler();

  // Tab creation triggers an update event due to the tab parented notification,
  // so the event handler issues two commits as well (one for tab creation, one
  // for tab update). During the first update, however, the tab is not syncable
  // and is hence skipped.
  auto tab_create_mock_batch = std::make_unique<StrictMock<MockWriteBatch>>();
  EXPECT_CALL(
      *tab_create_mock_batch,
      Put(Pointee(MatchesHeader(kSessionTag, {kWindowId1}, {kTabId1}))));
  EXPECT_CALL(*tab_create_mock_batch, Commit());

  auto navigation_mock_batch = std::make_unique<StrictMock<MockWriteBatch>>();
  EXPECT_CALL(*navigation_mock_batch,
              Put(Pointee(MatchesHeader(kSessionTag, {kWindowId1},
                                        {kTabId1, kTabId2}))));
  EXPECT_CALL(*navigation_mock_batch,
              Put(Pointee(MatchesTab(kSessionTag, kWindowId1, kTabId2,
                                     /*tab_node_id=*/_, /*urls=*/{kBar1}))));
  EXPECT_CALL(*navigation_mock_batch, Commit());

  EXPECT_CALL(mock_delegate_, CreateLocalSessionWriteBatch())
      .WillOnce(Return(ByMove(std::move(tab_create_mock_batch))))
      .WillOnce(Return(ByMove(std::move(navigation_mock_batch))));

  AddTab(kWindowId1, kBar1, kTabId2);
}

TEST_F(LocalSessionEventHandlerImplTest, PropagateNewWindow) {
  AddWindow(kWindowId1);
  AddTab(kWindowId1, kFoo1, kTabId1);
  AddTab(kWindowId1, kBar1, kTabId2);

  InitHandler();

  // Window creation triggers an update event due to the tab parented
  // notification, so the event handler issues two commits as well (one for
  // window creation, one for tab update). During the first update, however,
  // the window is not syncable and is hence skipped.
  auto tab_create_mock_batch = std::make_unique<StrictMock<MockWriteBatch>>();
  EXPECT_CALL(*tab_create_mock_batch,
              Put(Pointee(MatchesHeader(kSessionTag, {kWindowId1},
                                        {kTabId1, kTabId2}))));
  EXPECT_CALL(*tab_create_mock_batch, Commit());

  auto navigation_mock_batch = std::make_unique<StrictMock<MockWriteBatch>>();
  EXPECT_CALL(*navigation_mock_batch,
              Put(Pointee(MatchesHeader(kSessionTag, {kWindowId1, kWindowId2},
                                        {kTabId1, kTabId2, kTabId3}))));
  EXPECT_CALL(*navigation_mock_batch,
              Put(Pointee(MatchesTab(kSessionTag, kWindowId2, kTabId3,
                                     /*tab_node_id=*/_, /*urls=*/{kBaz1}))));
  EXPECT_CALL(*navigation_mock_batch, Commit());

  EXPECT_CALL(mock_delegate_, CreateLocalSessionWriteBatch())
      .WillOnce(Return(ByMove(std::move(tab_create_mock_batch))))
      .WillOnce(Return(ByMove(std::move(navigation_mock_batch))));

  AddWindow(kWindowId2);
  AddTab(kWindowId2, kBaz1, kTabId3);
}

TEST_F(LocalSessionEventHandlerImplTest,
       PropagateNewNavigationWithoutTabbedWindows) {
  const int kTabNodeId1 = 0;
  const int kTabNodeId2 = 1;

  // The tracker is initially restored from persisted state, containing two
  // custom tabs.
  sync_pb::SessionSpecifics custom_tab1;
  custom_tab1.set_session_tag(kSessionTag);
  custom_tab1.set_tab_node_id(kTabNodeId1);
  custom_tab1.mutable_tab()->set_window_id(kWindowId1);
  custom_tab1.mutable_tab()->set_tab_id(kTabId1);
  session_tracker_.ReassociateLocalTab(kTabNodeId1,
                                       SessionID::FromSerializedValue(kTabId1));
  UpdateTrackerWithSpecifics(custom_tab1, base::Time::Now(), &session_tracker_);

  sync_pb::SessionSpecifics custom_tab2;
  custom_tab2.set_session_tag(kSessionTag);
  custom_tab2.set_tab_node_id(kTabNodeId2);
  custom_tab2.mutable_tab()->set_window_id(kWindowId2);
  custom_tab2.mutable_tab()->set_tab_id(kTabId2);
  session_tracker_.ReassociateLocalTab(kTabNodeId2,
                                       SessionID::FromSerializedValue(kTabId2));
  UpdateTrackerWithSpecifics(custom_tab2, base::Time::Now(), &session_tracker_);

  sync_pb::SessionSpecifics header;
  header.set_session_tag(kSessionTag);
  header.mutable_header()->add_window()->set_window_id(kWindowId1);
  header.mutable_header()->mutable_window(0)->add_tab(kTabId1);
  header.mutable_header()->add_window()->set_window_id(kWindowId2);
  header.mutable_header()->mutable_window(1)->add_tab(kTabId2);
  UpdateTrackerWithSpecifics(header, base::Time::Now(), &session_tracker_);

  ASSERT_THAT(session_tracker_.LookupSession(kSessionTag),
              MatchesSyncedSession(kSessionTag,
                                   {{kWindowId1, std::vector<int>{kTabId1}},
                                    {kWindowId2, std::vector<int>{kTabId2}}}));

  AddWindow(kWindowId1, sync_pb::SessionWindow_BrowserType_TYPE_CUSTOM_TAB);
  TestSyncedTabDelegate* tab1 = AddTab(kWindowId1, kFoo1, kTabId1);
  tab1->SetSyncId(kTabNodeId1);

  AddWindow(kWindowId2, sync_pb::SessionWindow_BrowserType_TYPE_CUSTOM_TAB);
  AddTab(kWindowId2, kBar1, kTabId2)->SetSyncId(kTabNodeId2);

  InitHandler();

  auto update_mock_batch = std::make_unique<StrictMock<MockWriteBatch>>();
  // Note that the header is reported again, although it hasn't changed. This is
  // OK because sync will avoid updating an entity with identical content.
  EXPECT_CALL(*update_mock_batch,
              Put(Pointee(MatchesHeader(kSessionTag, {kWindowId1, kWindowId2},
                                        {kTabId1, kTabId2}))));
  EXPECT_CALL(
      *update_mock_batch,
      Put(Pointee(MatchesTab(kSessionTag, kWindowId1, kTabId1, kTabNodeId1,
                             /*urls=*/{kFoo1, kBaz1}))));
  EXPECT_CALL(*update_mock_batch, Commit());

  EXPECT_CALL(mock_delegate_, CreateLocalSessionWriteBatch())
      .WillOnce(Return(ByMove(std::move(update_mock_batch))));

  tab1->Navigate(kBaz1);
}

}  // namespace
}  // namespace sync_sessions
