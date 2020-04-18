// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_model_snapshot.h"

#import "ios/chrome/browser/tabs/legacy_tab_helper.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/web_state_list/fake_web_state_list_delegate.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_opener.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TabModelSnapshotTestTabMock : NSObject

@property(nonatomic, copy) NSString* tabId;
@property(nonatomic, copy) NSString* urlDisplayString;
@property(nonatomic, assign) double lastVisitedTimestamp;

@end

@implementation TabModelSnapshotTestTabMock

@synthesize tabId = _tabId;
@synthesize urlDisplayString = _urlDisplayString;
@synthesize lastVisitedTimestamp = _lastVisitedTimestamp;

@end

@interface TabModelSnapshotTestTabModelMock : NSObject
@end

@implementation TabModelSnapshotTestTabModelMock {
  FakeWebStateListDelegate _webStateListDelegate;
  std::unique_ptr<WebStateList> _webStateList;
}

- (instancetype)initWithTabs:(NSArray<Tab*>*)tabs {
  if ((self = [super init])) {
    _webStateList = std::make_unique<WebStateList>(&_webStateListDelegate);
    for (Tab* tab in tabs) {
      auto testWebState = std::make_unique<web::TestWebState>();
      LegacyTabHelper::CreateForWebStateForTesting(testWebState.get(), tab);
      _webStateList->InsertWebState(0, std::move(testWebState),
                                    WebStateList::INSERT_NO_FLAGS,
                                    WebStateOpener());
    }
  }
  return self;
}

- (WebStateList*)webStateList {
  return _webStateList.get();
}

@end

namespace {

class TabModelSnapshotTest : public PlatformTest {
 protected:
  Tab* TabMock(NSString* tabId, NSString* url, double time) {
    TabModelSnapshotTestTabMock* tabMock =
        [[TabModelSnapshotTestTabMock alloc] init];
    tabMock.tabId = tabId;
    tabMock.urlDisplayString = url;
    tabMock.lastVisitedTimestamp = time;
    return static_cast<Tab*>(tabMock);
  }
};

TEST_F(TabModelSnapshotTest, TestSingleHash) {
  Tab* tab1 = TabMock(@"id1", @"url1", 12345.6789);
  Tab* tab2 = TabMock(@"id2", @"url1", 12345.6789);
  Tab* tab3 = TabMock(@"id1", @"url2", 12345.6789);
  Tab* tab4 = TabMock(@"id1", @"url1", 12345);

  // Same tab
  size_t hash1 = TabModelSnapshot::HashOfTheVisiblePropertiesOfATab(tab1);
  size_t hash2 = TabModelSnapshot::HashOfTheVisiblePropertiesOfATab(tab1);
  EXPECT_EQ(hash1, hash2);

  // Different ids
  size_t hash3 = TabModelSnapshot::HashOfTheVisiblePropertiesOfATab(tab1);
  size_t hash4 = TabModelSnapshot::HashOfTheVisiblePropertiesOfATab(tab2);
  EXPECT_NE(hash3, hash4);

  // Different urls
  size_t hash5 = TabModelSnapshot::HashOfTheVisiblePropertiesOfATab(tab1);
  size_t hash6 = TabModelSnapshot::HashOfTheVisiblePropertiesOfATab(tab3);
  EXPECT_NE(hash5, hash6);

  // Different timestamps
  size_t hash7 = TabModelSnapshot::HashOfTheVisiblePropertiesOfATab(tab1);
  size_t hash8 = TabModelSnapshot::HashOfTheVisiblePropertiesOfATab(tab4);
  EXPECT_NE(hash7, hash8);
}

TEST_F(TabModelSnapshotTest, TestSnapshotHashes) {
  Tab* tab1 = TabMock(@"id1", @"url1", 12345.6789);
  Tab* tab2 = TabMock(@"id2", @"url1", 12345.6789);

  TabModelSnapshotTestTabModelMock* tabModel =
      [[TabModelSnapshotTestTabModelMock alloc] initWithTabs:@[ tab1, tab2 ]];
  TabModelSnapshot tabModelSnapshot(static_cast<TabModel*>(tabModel));

  EXPECT_EQ(tabModelSnapshot.hashes().size(), 2UL);
  EXPECT_EQ(tabModelSnapshot.hashes()[0],
            TabModelSnapshot::HashOfTheVisiblePropertiesOfATab(tab1));
  EXPECT_EQ(tabModelSnapshot.hashes()[1],
            TabModelSnapshot::HashOfTheVisiblePropertiesOfATab(tab2));
}

}  // namespace
