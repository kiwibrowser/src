// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/reading_list_coordinator.h"

#include <memory>

#include "base/time/default_clock.h"
#include "components/favicon/core/large_icon_service.h"
#include "components/favicon/core/test/mock_favicon_service.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"
#include "components/feature_engagement/test/mock_tracker.h"
#include "components/reading_list/core/reading_list_entry.h"
#include "components/reading_list/core/reading_list_model_impl.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/feature_engagement/tracker_factory.h"
#include "ios/chrome/browser/reading_list/offline_url_utils.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_controller.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_item.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_mediator.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_utils.h"
#import "ios/chrome/browser/ui/url_loader.h"
#import "ios/chrome/test/fakes/fake_url_loader.h"
#include "ios/web/public/referrer.h"
#import "ios/web/public/test/web_test_with_web_state.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"
#include "ui/base/page_transition_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using favicon::PostReply;
using testing::_;

#pragma mark - ReadingListCoordinatorTest

class ReadingListCoordinatorTest : public web::WebTestWithWebState {
 public:
  ReadingListCoordinatorTest() {
    url_loader_ = [[FakeURLLoader alloc] init];

    TestChromeBrowserState::Builder builder;
    builder.AddTestingFactory(
        feature_engagement::TrackerFactory::GetInstance(),
        ReadingListCoordinatorTest::BuildFeatureEngagementMockTracker);
    browser_state_ = builder.Build();

    reading_list_model_.reset(new ReadingListModelImpl(
        nullptr, nullptr, base::DefaultClock::GetInstance()));
    large_icon_service_.reset(new favicon::LargeIconService(
        &mock_favicon_service_, /*image_fetcher=*/nullptr));
    mediator_ =
        [[ReadingListMediator alloc] initWithModel:reading_list_model_.get()
                                  largeIconService:large_icon_service_.get()];
    coordinator_ = [[ReadingListCoordinator alloc]
        initWithBaseViewController:nil
                      browserState:browser_state_.get()
                            loader:url_loader_];
    coordinator_.mediator = mediator_;

    EXPECT_CALL(mock_favicon_service_,
                GetLargestRawFaviconForPageURL(_, _, _, _, _))
        .WillRepeatedly(PostReply<5>(favicon_base::FaviconRawBitmapResult()));
  }

  ~ReadingListCoordinatorTest() override {}

  ReadingListCoordinator* GetCoordinator() { return coordinator_; }

  ReadingListModel* GetReadingListModel() { return reading_list_model_.get(); }
  FakeURLLoader* GetLoader() { return url_loader_; }

  ios::ChromeBrowserState* GetBrowserState() { return browser_state_.get(); }

  ReadingListCollectionViewController*
  GetAReadingListCollectionViewController() {
    return [[ReadingListCollectionViewController alloc]
        initWithDataSource:mediator_
                   toolbar:nil];
  }

  static std::unique_ptr<KeyedService> BuildFeatureEngagementMockTracker(
      web::BrowserState*) {
    return std::make_unique<feature_engagement::test::MockTracker>();
  }

 private:
  ReadingListCoordinator* coordinator_;
  ReadingListMediator* mediator_;
  std::unique_ptr<ReadingListModelImpl> reading_list_model_;
  FakeURLLoader* url_loader_;
  testing::StrictMock<favicon::MockFaviconService> mock_favicon_service_;
  std::unique_ptr<favicon::LargeIconService> large_icon_service_;
  std::unique_ptr<TestChromeBrowserState> browser_state_;
};

// Tests that the implementation of ReadingListCoordinator openItemAtIndexPath
// opens the entry.
TEST_F(ReadingListCoordinatorTest, OpenItem) {
  // Setup.
  GURL url("https://chromium.org");
  std::string title("Chromium");
  ReadingListModel* model = GetReadingListModel();
  model->AddEntry(url, title, reading_list::ADDED_VIA_CURRENT_APP);

  ReadingListCollectionViewItem* item = [[ReadingListCollectionViewItem alloc]
           initWithType:0
                    url:url
      distillationState:ReadingListUIDistillationStatusSuccess];

  // Action.
  [GetCoordinator() readingListCollectionViewController:
                        GetAReadingListCollectionViewController()
                                               openItem:item];

  // Tests.
  FakeURLLoader* loader = GetLoader();
  EXPECT_EQ(url, loader.url);
  EXPECT_TRUE(ui::PageTransitionCoreTypeIs(ui::PAGE_TRANSITION_AUTO_BOOKMARK,
                                           loader.transition));
  EXPECT_EQ(NO, loader.rendererInitiated);
}

TEST_F(ReadingListCoordinatorTest, OpenItemOffline) {
  // Setup.
  GURL url("https://chromium.org");
  std::string title("Chromium");
  ReadingListModel* model = GetReadingListModel();
  model->AddEntry(url, title, reading_list::ADDED_VIA_CURRENT_APP);
  base::FilePath distilled_path("test");
  GURL distilled_url("https://distilled.com");
  model->SetEntryDistilledInfo(url, distilled_path, distilled_url, 123,
                               base::Time::FromTimeT(10));

  ReadingListCollectionViewItem* item = [[ReadingListCollectionViewItem alloc]
           initWithType:0
                    url:url
      distillationState:ReadingListUIDistillationStatusSuccess];
  GURL offlineURL =
      reading_list::OfflineURLForPath(distilled_path, url, distilled_url);
  ASSERT_FALSE(model->GetEntryByURL(url)->IsRead());

  // Action.
  [GetCoordinator() readingListCollectionViewController:
                        GetAReadingListCollectionViewController()
                                openItemOfflineInNewTab:item];

  // Tests.
  FakeURLLoader* loader = GetLoader();
  EXPECT_EQ(offlineURL, loader.url);
  EXPECT_FALSE(loader.inIncognito);
  EXPECT_TRUE(model->GetEntryByURL(url)->IsRead());
}

TEST_F(ReadingListCoordinatorTest, OpenItemInNewTab) {
  // Setup.
  GURL url("https://chromium.org");
  std::string title("Chromium");
  ReadingListModel* model = GetReadingListModel();
  model->AddEntry(url, title, reading_list::ADDED_VIA_CURRENT_APP);

  ReadingListCollectionViewItem* item = [[ReadingListCollectionViewItem alloc]
           initWithType:0
                    url:url
      distillationState:ReadingListUIDistillationStatusSuccess];

  // Action.
  [GetCoordinator() readingListCollectionViewController:
                        GetAReadingListCollectionViewController()
                                       openItemInNewTab:item
                                              incognito:YES];

  // Tests.
  FakeURLLoader* loader = GetLoader();
  EXPECT_EQ(url, loader.url);
  EXPECT_TRUE(loader.inIncognito);
}

TEST_F(ReadingListCoordinatorTest, SendViewedReadingListEventInStart) {
  // Setup.
  feature_engagement::test::MockTracker* tracker =
      static_cast<feature_engagement::test::MockTracker*>(
          feature_engagement::TrackerFactory::GetForBrowserState(
              GetBrowserState()));

  // Actions and Tests.
  EXPECT_CALL((*tracker),
              NotifyEvent(feature_engagement::events::kViewedReadingList));
  [GetCoordinator() start];
}
