// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/legacy_history_collection_view_controller.h"

#include <memory>

#include "base/callback.h"
#include "base/strings/string16.h"
#import "base/test/ios/wait_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/history/core/browser/browsing_history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/signin/authentication_service_factory.h"
#include "ios/chrome/browser/signin/authentication_service_fake.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_test_util.h"
#include "ios/chrome/browser/sync/sync_setup_service.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service_mock.h"
#import "ios/chrome/browser/ui/history/ios_browsing_history_driver.h"
#include "ios/chrome/browser/ui/history/ios_browsing_history_driver.h"
#include "ios/chrome/browser/ui/ui_feature_flags.h"
#import "ios/chrome/browser/ui/url_loader.h"
#include "ios/chrome/test/block_cleanup_test.h"
#include "ios/web/public/test/test_web_thread.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using base::Time;
using base::TimeDelta;
using history::BrowsingHistoryService;

namespace {

const char kTestUrl1[] = "http://test1/";
const char kTestUrl2[] = "http://test2/";

std::vector<BrowsingHistoryService::HistoryEntry> QueryResultWithVisits(
    std::vector<std::pair<const GURL&, Time>> visits) {
  std::vector<BrowsingHistoryService::HistoryEntry> entries;
  for (std::pair<const GURL&, Time> visit : visits) {
    BrowsingHistoryService::HistoryEntry entry;
    entry.url = visit.first;
    entry.time = visit.second;
    entries.push_back(entry);
  }
  return entries;
}

std::unique_ptr<KeyedService> BuildMockSyncSetupService(
    web::BrowserState* context) {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  syncer::SyncService* sync_service =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(browser_state);
  return std::make_unique<SyncSetupServiceMock>(sync_service,
                                                browser_state->GetPrefs());
}

}  // namespace

@interface LegacyHistoryCollectionViewController (Testing)<HistoryConsumer>
- (void)didPressClearBrowsingBar;
@end

class LegacyHistoryCollectionViewControllerTest : public BlockCleanupTest {
 public:
  LegacyHistoryCollectionViewControllerTest() {}
  ~LegacyHistoryCollectionViewControllerTest() override {}

  void SetUp() override {
    BlockCleanupTest::SetUp();
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    TestChromeBrowserState::Builder builder;
    builder.AddTestingFactory(
        AuthenticationServiceFactory::GetInstance(),
        AuthenticationServiceFake::CreateAuthenticationService);
    builder.AddTestingFactory(SyncSetupServiceFactory::GetInstance(),
                              &BuildMockSyncSetupService);
    mock_browser_state_ = builder.Build();
    sync_setup_service_mock_ = static_cast<SyncSetupServiceMock*>(
        SyncSetupServiceFactory::GetForBrowserState(mock_browser_state_.get()));
    mock_delegate_ = [OCMockObject
        niceMockForProtocol:@protocol(
                                LegacyHistoryCollectionViewControllerDelegate)];
    mock_url_loader_ = [OCMockObject niceMockForProtocol:@protocol(UrlLoader)];
    history_collection_view_controller_ =
        [[LegacyHistoryCollectionViewController alloc]
            initWithLoader:mock_url_loader_
              browserState:mock_browser_state_.get()
                  delegate:mock_delegate_];

    _browsingHistoryDriver = std::make_unique<IOSBrowsingHistoryDriver>(
        mock_browser_state_.get(), history_collection_view_controller_);

    _browsingHistoryService = std::make_unique<history::BrowsingHistoryService>(
        _browsingHistoryDriver.get(),
        ios::HistoryServiceFactory::GetForBrowserState(
            mock_browser_state_.get(), ServiceAccessType::EXPLICIT_ACCESS),
        IOSChromeProfileSyncServiceFactory::GetForBrowserState(
            mock_browser_state_.get()));

    history_collection_view_controller_.historyService =
        _browsingHistoryService.get();
  }

  void TearDown() override {
    history_collection_view_controller_ = nil;
    BlockCleanupTest::TearDown();
  }

  void QueryHistory(std::vector<std::pair<const GURL&, Time>> visits) {
    std::vector<BrowsingHistoryService::HistoryEntry> results =
        QueryResultWithVisits(visits);
    BrowsingHistoryService::QueryResultsInfo query_results_info;
    query_results_info.reached_beginning = true;
    [history_collection_view_controller_
        historyQueryWasCompletedWithResults:results
                           queryResultsInfo:query_results_info
                        continuationClosure:base::OnceClosure()];
  }

 protected:
  web::TestWebThreadBundle thread_bundle_;
  id<UrlLoader> mock_url_loader_;
  std::unique_ptr<TestChromeBrowserState> mock_browser_state_;
  id<LegacyHistoryCollectionViewControllerDelegate> mock_delegate_;
  LegacyHistoryCollectionViewController* history_collection_view_controller_;
  bool privacy_settings_opened_;
  SyncSetupServiceMock* sync_setup_service_mock_;
  DISALLOW_COPY_AND_ASSIGN(LegacyHistoryCollectionViewControllerTest);
  std::unique_ptr<IOSBrowsingHistoryDriver> _browsingHistoryDriver;
  std::unique_ptr<history::BrowsingHistoryService> _browsingHistoryService;
};

// Tests that isEmpty property returns NO after entries have been received.
TEST_F(LegacyHistoryCollectionViewControllerTest, IsEmpty) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUIRefreshPhase1);
  QueryHistory({{GURL(kTestUrl1), Time::Now()}});
  EXPECT_FALSE([history_collection_view_controller_ isEmpty]);
}

// Tests that local history items are shown when sync is enabled,
// HISTORY_DELETE_DIRECTIVES is enabled, and sync_returned is false.
// This ensures that when HISTORY_DELETE_DIRECTIVES is disabled,
// only local device history items are shown.
TEST_F(LegacyHistoryCollectionViewControllerTest, IsNotEmptyWhenSyncEnabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUIRefreshPhase1);
  EXPECT_CALL(*sync_setup_service_mock_, IsSyncEnabled())
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(*sync_setup_service_mock_,
              IsDataTypeActive(syncer::HISTORY_DELETE_DIRECTIVES))
      .WillRepeatedly(testing::Return(false));

  QueryHistory({{GURL(kTestUrl1), Time::Now()}});
  EXPECT_FALSE([history_collection_view_controller_ isEmpty]);
}

// Tests adding two entries to history from the same day, then deleting the
// first of them results in one history entry in the collection.
TEST_F(LegacyHistoryCollectionViewControllerTest, DeleteSingleEntry) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUIRefreshPhase1);
  QueryHistory(
      {{GURL(kTestUrl1), Time::Now()}, {GURL(kTestUrl2), Time::Now()}});

  UICollectionView* collection_view =
      [history_collection_view_controller_ collectionView];
  [collection_view
      selectItemAtIndexPath:[NSIndexPath indexPathForItem:0 inSection:1]
                   animated:NO
             scrollPosition:UICollectionViewScrollPositionNone];
  [history_collection_view_controller_ deleteSelectedItemsFromHistory];

  // Expect header section and one entries section with one item.
  EXPECT_EQ(2, [collection_view numberOfSections]);
  EXPECT_EQ(1, [collection_view numberOfItemsInSection:1]);
}

// Tests that adding two entries to history from the same day then deleting
// both of them results in only the header section in the collection.
TEST_F(LegacyHistoryCollectionViewControllerTest, DeleteMultipleEntries) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUIRefreshPhase1);

  QueryHistory(
      {{GURL(kTestUrl1), Time::Now()}, {GURL(kTestUrl2), Time::Now()}});

  // Select history entries and tap delete.
  UICollectionView* collection_view =
      [history_collection_view_controller_ collectionView];
  collection_view.allowsMultipleSelection = YES;
  [collection_view
      selectItemAtIndexPath:[NSIndexPath indexPathForItem:0 inSection:1]
                   animated:NO
             scrollPosition:UICollectionViewScrollPositionNone];
  [collection_view
      selectItemAtIndexPath:[NSIndexPath indexPathForItem:1 inSection:1]
                   animated:NO
             scrollPosition:UICollectionViewScrollPositionNone];
  EXPECT_EQ(2, (int)[[collection_view indexPathsForSelectedItems] count]);
  [history_collection_view_controller_ deleteSelectedItemsFromHistory];

  // Expect only the header section to remain.
  EXPECT_EQ(1, [collection_view numberOfSections]);
}

// Tests that adding two entries to history from different days then deleting
// both of them results in only the header section in the collection.
TEST_F(LegacyHistoryCollectionViewControllerTest, DeleteMultipleSections) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUIRefreshPhase1);
  QueryHistory({{GURL(kTestUrl1), Time::Now() - TimeDelta::FromDays(1)},
                {GURL(kTestUrl2), Time::Now()}});

  UICollectionView* collection_view =
      [history_collection_view_controller_ collectionView];
  // Expect two history sections in addition to the header section.
  EXPECT_EQ(3, [collection_view numberOfSections]);

  // Select all history items, and delete.
  collection_view.allowsMultipleSelection = YES;
  [collection_view
      selectItemAtIndexPath:[NSIndexPath indexPathForItem:0 inSection:1]
                   animated:NO
             scrollPosition:UICollectionViewScrollPositionNone];
  [collection_view
      selectItemAtIndexPath:[NSIndexPath indexPathForItem:0 inSection:2]
                   animated:NO
             scrollPosition:UICollectionViewScrollPositionNone];
  [history_collection_view_controller_ deleteSelectedItemsFromHistory];

  // Expect only the header section to remain.
  EXPECT_EQ(1, [collection_view numberOfSections]);
}
