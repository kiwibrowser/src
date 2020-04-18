// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/tab_switcher/tab_switcher_model.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/ui/ntp/recent_tabs/synced_sessions.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_model_private.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_utils.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// A lightweight DistantTab.
class LightDT {
 public:
  LightDT(std::string const& urlSuffix, std::string const& title = "")
      : url("http://www.foo.com/" + urlSuffix), title(title) {}
  std::string url;
  std::string title;
};

// A lightweight DistantSession.
struct LightDS {
  std::string tag;
  std::vector<LightDT> distantTabs;
};

// A lightweight SyncedSessions.
using LightSS = std::vector<LightDS>;

}  // namespace

// Helper class to test calls to the TabSwitcherModelDelegate.
@interface DelegateTester : NSObject<TabSwitcherModelDelegate>
@end

@implementation DelegateTester {
  NSArray* _expectedSessionRemoved;
  NSArray* _expectedSessionInserted;
  std::set<std::string> _expectedTagsOfTheSessionsNeedingUpdates;
}

- (void)expectSessionsRemoved:(NSArray*)expectedIndexes {
  _expectedSessionRemoved = expectedIndexes;
}

- (void)expectSessionsInserted:(NSArray*)expectedIndexes {
  _expectedSessionInserted = expectedIndexes;
}

- (void)expectSessionMayNeedUpdate:(std::set<std::string> const&)tags {
  _expectedTagsOfTheSessionsNeedingUpdates = tags;
}

- (void)verify {
  EXPECT_EQ(0UL, [_expectedSessionRemoved count]);
  EXPECT_EQ(0UL, [_expectedSessionInserted count]);
  EXPECT_EQ(0UL, _expectedTagsOfTheSessionsNeedingUpdates.size());
}

#pragma mark - TabSwitcherModelDelegate

- (void)distantSessionsRemovedAtSortedIndexes:(NSArray*)removedIndexes
                      insertedAtSortedIndexes:(NSArray*)insertedIndexes {
  EXPECT_TRUE(removedIndexes == _expectedSessionRemoved ||
              [removedIndexes isEqualToArray:_expectedSessionRemoved]);
  EXPECT_TRUE(insertedIndexes == _expectedSessionInserted ||
              [insertedIndexes isEqualToArray:_expectedSessionInserted]);
  _expectedSessionRemoved = nil;
  _expectedSessionInserted = nil;
}

- (void)distantSessionMayNeedUpdate:(std::string const&)tag {
  EXPECT_EQ(1UL, _expectedTagsOfTheSessionsNeedingUpdates.erase(tag));
}

- (void)localSessionMayNeedUpdate:(TabSwitcherSessionType)type {
  NOTREACHED();
}

- (void)signInPanelChangedTo:(TabSwitcherSignInPanelsType)panelType {
  NOTREACHED();
}

- (CGSize)sizeForItemAtIndex:(NSUInteger)index
                   inSession:(TabSwitcherSessionType)session {
  return CGSizeZero;
}

- (BOOL)isSigninInProgress {
  NOTREACHED();
  return NO;
}

@end

namespace {

class TabSwitcherModelTest : public PlatformTest {
 protected:
  void SetUp() override { delegate_ = [[DelegateTester alloc] init]; }

  void AddSessionToSessions(synced_sessions::SyncedSessions& sessions,
                            std::string const& session_tag,
                            std::vector<size_t> const& tab_ids) {
    std::vector<LightDT> light_distant_tabs;
    for (size_t tab_id : tab_ids) {
      light_distant_tabs.push_back(LightDT(std::to_string(tab_id), ""));
    }
    AddDetailedSessionToSessions(sessions, session_tag, light_distant_tabs);
  }

  std::unique_ptr<synced_sessions::SyncedSessions> syncedSessionsFromTestData(
      LightSS const& lightSyncedSessions) {
    auto syncedSessions = std::make_unique<synced_sessions::SyncedSessions>();
    for (auto& lightdistantSession : lightSyncedSessions) {
      AddDetailedSessionToSessions(*syncedSessions, lightdistantSession.tag,
                                   lightdistantSession.distantTabs);
    }
    return syncedSessions;
  }

  void AddDetailedSessionToSessions(
      synced_sessions::SyncedSessions& sessions,
      std::string const& session_tag,
      std::vector<LightDT> const& light_distant_tabs) {
    // Create a new DistantSession and initialize it with |sessionTag| and
    // |tabTags|.
    auto distant_session = std::make_unique<synced_sessions::DistantSession>();
    distant_session->tag = session_tag;

    for (auto const& light_distant_tab : light_distant_tabs) {
      auto temp_tab = std::make_unique<synced_sessions::DistantTab>();
      temp_tab->virtual_url = GURL(light_distant_tab.url);
      temp_tab->title = base::ASCIIToUTF16(light_distant_tab.title);
      distant_session->tabs.push_back(std::move(temp_tab));
    }

    sessions.AddDistantSessionForTest(std::move(distant_session));
  }
  DelegateTester* delegate_;
  TabSwitcherModel* model_;
};

TEST_F(TabSwitcherModelTest, TestNoDiffs) {
  // Test with 2 empty sessions.
  synced_sessions::SyncedSessions old_sessions_1;
  synced_sessions::SyncedSessions new_sessions_1;
  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:old_sessions_1
                                to:new_sessions_1];
  [delegate_ verify];

  // Test with 2 identical sessions.
  synced_sessions::SyncedSessions old_sessions_2;
  synced_sessions::SyncedSessions new_sessions_2;
  AddSessionToSessions(old_sessions_2, "Foo", {0, 1});
  AddSessionToSessions(new_sessions_2, "Foo", {0, 1});
  [delegate_ expectSessionMayNeedUpdate:{"Foo"}];

  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:old_sessions_2
                                to:new_sessions_2];
  [delegate_ verify];
}

TEST_F(TabSwitcherModelTest, TestSessionDiffs) {
  // Test with 1 session inserted.
  synced_sessions::SyncedSessions old_sessions_1;
  synced_sessions::SyncedSessions new_sessions_1;
  AddSessionToSessions(new_sessions_1, "Foo", {0});
  [delegate_ expectSessionsInserted:@[ @0 ]];
  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:old_sessions_1
                                to:new_sessions_1];
  [delegate_ verify];

  // Test with 2 sessions inserted.
  synced_sessions::SyncedSessions old_sessions_2;
  synced_sessions::SyncedSessions new_sessions_2;
  AddSessionToSessions(old_sessions_2, "Bar", {0});
  AddSessionToSessions(new_sessions_2, "Foo", {0});
  AddSessionToSessions(new_sessions_2, "Bar", {0});
  AddSessionToSessions(new_sessions_2, "Qux", {0});
  [delegate_ expectSessionsInserted:@[ @0, @2 ]];
  [delegate_ expectSessionMayNeedUpdate:{"Bar"}];
  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:old_sessions_2
                                to:new_sessions_2];
  [delegate_ verify];

  // Test with 2 sessions removed.
  synced_sessions::SyncedSessions old_sessions_3;
  synced_sessions::SyncedSessions new_sessions_3;
  AddSessionToSessions(old_sessions_3, "Foo", {0});
  AddSessionToSessions(old_sessions_3, "Bar", {0});
  AddSessionToSessions(old_sessions_3, "Qux", {0});
  AddSessionToSessions(new_sessions_3, "Bar", {0});
  [delegate_ expectSessionsRemoved:@[ @0, @2 ]];
  [delegate_ expectSessionMayNeedUpdate:{"Bar"}];
  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:old_sessions_3
                                to:new_sessions_3];
  [delegate_ verify];

  // Test with 2 sessions inserted, 2 removed.
  synced_sessions::SyncedSessions old_sessions_4;
  synced_sessions::SyncedSessions new_sessions_4;
  AddSessionToSessions(old_sessions_4, "Deleted1", {0});
  AddSessionToSessions(old_sessions_4, "a", {0});
  AddSessionToSessions(old_sessions_4, "Delete2", {0});

  AddSessionToSessions(new_sessions_4, "b", {0});
  AddSessionToSessions(new_sessions_4, "a", {0});
  AddSessionToSessions(new_sessions_4, "c", {0});
  [delegate_ expectSessionsRemoved:@[ @0, @2 ]];
  [delegate_ expectSessionsInserted:@[ @0, @2 ]];
  [delegate_ expectSessionMayNeedUpdate:{"a"}];

  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:old_sessions_4
                                to:new_sessions_4];
  [delegate_ verify];
}

TEST_F(TabSwitcherModelTest, TestTabsSimpleDiffs) {
  // Test with 2 tabs added.
  synced_sessions::SyncedSessions old_sessions_1;
  synced_sessions::SyncedSessions new_sessions_1;
  AddSessionToSessions(old_sessions_1, "Bar", {100});
  AddSessionToSessions(old_sessions_1, "Foo", {100});
  AddSessionToSessions(new_sessions_1, "Bar", {100});
  AddSessionToSessions(new_sessions_1, "Foo", {99, 100, 101});
  [delegate_ expectSessionMayNeedUpdate:{"Bar", "Foo"}];

  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:old_sessions_1
                                to:new_sessions_1];
  [delegate_ verify];

  // Test with 2 tabs removed.
  synced_sessions::SyncedSessions old_sessions_2;
  synced_sessions::SyncedSessions new_sessions_2;
  AddSessionToSessions(old_sessions_2, "Bar", {0});
  AddSessionToSessions(old_sessions_2, "Foo", {100, 101, 102});
  AddSessionToSessions(new_sessions_2, "Bar", {0});
  AddSessionToSessions(new_sessions_2, "Foo", {100});
  [delegate_ expectSessionMayNeedUpdate:{"Bar", "Foo"}];

  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:old_sessions_2
                                to:new_sessions_2];
  [delegate_ verify];

  // Test with 2 tabs updated.
  synced_sessions::SyncedSessions old_sessions_3;
  synced_sessions::SyncedSessions new_sessions_3;
  std::vector<LightDT> old_tabs_3;
  old_tabs_3.push_back(LightDT("1", "1"));
  old_tabs_3.push_back(LightDT("2", "2"));
  old_tabs_3.push_back(LightDT("3", "3"));
  std::vector<LightDT> new_tabs_3;
  new_tabs_3.push_back(LightDT("1", "1 bis"));
  new_tabs_3.push_back(LightDT("2", "2"));
  new_tabs_3.push_back(LightDT("a", "3"));
  AddDetailedSessionToSessions(old_sessions_3, "Foo", old_tabs_3);
  AddDetailedSessionToSessions(new_sessions_3, "Foo", new_tabs_3);
  [delegate_ expectSessionMayNeedUpdate:{"Foo"}];
  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:old_sessions_3
                                to:new_sessions_3];
  [delegate_ verify];
}

TEST_F(TabSwitcherModelTest, TestTabsDeletionInsertions) {
  synced_sessions::SyncedSessions old_sessions;
  synced_sessions::SyncedSessions new_sessions;
  std::vector<LightDT> old_tabs;
  old_tabs.push_back(LightDT("0", "0"));
  old_tabs.push_back(LightDT("1", "1"));
  old_tabs.push_back(LightDT("2", "2"));
  old_tabs.push_back(LightDT("3", "3"));
  std::vector<LightDT> new_tabs;
  new_tabs.push_back(LightDT("1", "1"));
  new_tabs.push_back(LightDT("2", "2"));
  new_tabs.push_back(LightDT("3", "3"));
  new_tabs.push_back(LightDT("0", "0"));
  AddDetailedSessionToSessions(old_sessions, "Foo", old_tabs);
  AddDetailedSessionToSessions(new_sessions, "Foo", new_tabs);
  [delegate_ expectSessionMayNeedUpdate:{"Foo"}];
  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:old_sessions
                                to:new_sessions];
  [delegate_ verify];
}

TEST_F(TabSwitcherModelTest, TestTabsWithInterleavedDiffs) {
  synced_sessions::SyncedSessions old_sessions;
  synced_sessions::SyncedSessions new_sessions;
  std::vector<LightDT> old_tabs;
  old_tabs.push_back(LightDT("0", "0"));
  old_tabs.push_back(LightDT("1", "1"));  // gets deleted
  old_tabs.push_back(LightDT("2", "2"));
  old_tabs.push_back(LightDT("3", "3"));  // gets updated
  old_tabs.push_back(LightDT("4", "4"));
  old_tabs.push_back(LightDT("6", "6"));
  old_tabs.push_back(LightDT("7", "7"));  // gets updated
  std::vector<LightDT> new_tabs;
  new_tabs.push_back(LightDT("0", "0"));
  new_tabs.push_back(LightDT("2", "2"));
  new_tabs.push_back(LightDT("3", "3 bis"));
  new_tabs.push_back(LightDT("4", "4"));
  new_tabs.push_back(LightDT("5", "5"));  // is inserted
  new_tabs.push_back(LightDT("6", "6"));
  new_tabs.push_back(LightDT("7", "7 bis"));
  AddDetailedSessionToSessions(old_sessions, "Foo", old_tabs);
  AddDetailedSessionToSessions(new_sessions, "Foo", new_tabs);
  [delegate_ expectSessionMayNeedUpdate:{"Foo"}];
  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:old_sessions
                                to:new_sessions];
  [delegate_ verify];
}

// Tests that the reordering of sessions does not result in calls to the
// delegate.
TEST_F(TabSwitcherModelTest, TestReorderingOfSessions) {
  std::vector<LightDS> old_sessions_data = {
      {"A", {LightDT("a"), LightDT("b")}},
      {"B", {LightDT("a"), LightDT("b")}},
      {"C", {LightDT("a"), LightDT("b")}},
  };
  std::vector<LightDS> new_sessions_data = {
      {"C", {LightDT("a"), LightDT("b")}},
      {"A", {LightDT("a"), LightDT("b")}},
      {"B", {LightDT("a"), LightDT("b")}},
  };
  auto old_sessions = syncedSessionsFromTestData(old_sessions_data);
  auto new_sessions = syncedSessionsFromTestData(new_sessions_data);
  [delegate_ expectSessionMayNeedUpdate:{"A", "B", "C"}];
  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:*old_sessions
                                to:*new_sessions];
  [delegate_ verify];
}

// Tests that the reordering of sessions does not result in wrong deletion
// indexes.
TEST_F(TabSwitcherModelTest, TestReorderingOfSessionsWithDeletion) {
  std::vector<LightDS> old_sessions_data = {
      {"A", {LightDT("a"), LightDT("b"), LightDT("c")}},
      {"B", {LightDT("a"), LightDT("b")}},  // deleted
      {"C", {LightDT("a"), LightDT("b")}},
      {"D", {LightDT("a"), LightDT("b")}},  // deleted
  };
  std::vector<LightDS> new_sessions_data = {
      {"C", {LightDT("a"), LightDT("b")}},
      {"A", {LightDT("a"), LightDT("b"), LightDT("c")}},
  };
  auto old_sessions = syncedSessionsFromTestData(old_sessions_data);
  auto new_sessions = syncedSessionsFromTestData(new_sessions_data);
  [delegate_ expectSessionsRemoved:@[ @1, @3 ]];
  [delegate_ expectSessionMayNeedUpdate:{"A", "C"}];
  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:*old_sessions
                                to:*new_sessions];
  [delegate_ verify];
}

// Tests that the reordering of sessions does not result in wrong insertion
// indexes.
TEST_F(TabSwitcherModelTest, TestReorderingOfSessionsWithInsertion) {
  std::vector<LightDS> old_sessions_data = {
      {"A", {LightDT("a"), LightDT("b")}},
      {"B", {LightDT("a"), LightDT("b")}},
      {"C", {LightDT("a"), LightDT("b")}},
  };
  std::vector<LightDS> new_sessions_data = {
      {"B", {LightDT("a"), LightDT("b")}},
      {"D", {LightDT("a"), LightDT("b")}},  // inserted
      {"C", {LightDT("a"), LightDT("b")}},
      {"A", {LightDT("a"), LightDT("b")}},
      {"E", {LightDT("a"), LightDT("b")}},  // inserted
  };
  auto old_sessions = syncedSessionsFromTestData(old_sessions_data);
  auto new_sessions = syncedSessionsFromTestData(new_sessions_data);
  [delegate_ expectSessionsInserted:@[ @1, @4 ]];
  [delegate_ expectSessionMayNeedUpdate:{"A", "B", "C"}];
  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:*old_sessions
                                to:*new_sessions];
  [delegate_ verify];
}

TEST_F(TabSwitcherModelTest, TestTabsWithSessionAndTabChanges) {
  std::vector<LightDS> old_sessions_data = {
      {"0", {LightDT("a"), LightDT("b")}},
      {"1", {LightDT("a"), LightDT("b")}},  // session deleted
      {"2", {LightDT("a"), LightDT("b"), LightDT("c")}},
      {"3", {LightDT("b")}},
      {"4", {LightDT("a"), LightDT("b"), LightDT("c")}},  // tab deleted
      {"5", {LightDT("a"), LightDT("b")}},
  };

  std::vector<LightDS> new_sessions_data = {
      {"A", {LightDT("a")}},  // session inserted
      {"0", {LightDT("a"), LightDT("b")}},
      {"2", {LightDT("A"), LightDT("b"), LightDT("C")}},  // tabs updated
      {"3", {LightDT("a"), LightDT("b")}},                // tab inserted
      {"4", {LightDT("b")}},
      {"5", {LightDT("a"), LightDT("b")}},
  };

  auto old_sessions = syncedSessionsFromTestData(old_sessions_data);
  auto new_sessions = syncedSessionsFromTestData(new_sessions_data);

  [delegate_ expectSessionMayNeedUpdate:{"0", "2", "3", "4", "5"}];
  [delegate_ expectSessionsRemoved:@[ @1 ]];
  [delegate_ expectSessionsInserted:@[ @0 ]];

  [TabSwitcherModel notifyDelegate:delegate_
                   aboutChangeFrom:*old_sessions
                                to:*new_sessions];
  [delegate_ verify];
}

}  // namespace
