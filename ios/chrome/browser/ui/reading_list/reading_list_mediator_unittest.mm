// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/reading_list_mediator.h"

#include <memory>

#include "base/strings/sys_string_conversions.h"
#include "base/test/simple_test_clock.h"
#include "components/favicon/core/large_icon_service.h"
#include "components/favicon/core/test/mock_favicon_service.h"
#include "components/reading_list/core/reading_list_model_impl.h"
#include "components/url_formatter/url_formatter.h"
#include "ios/chrome/browser/favicon/ios_chrome_large_icon_service_factory.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_item.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_item_accessibility_delegate.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using testing::_;

class ReadingListMediatorTest : public PlatformTest {
 public:
  ReadingListMediatorTest() {
    model_ = std::make_unique<ReadingListModelImpl>(nullptr, nullptr, &clock_);
    EXPECT_CALL(mock_favicon_service_,
                GetLargestRawFaviconForPageURL(_, _, _, _, _))
        .WillRepeatedly(
            favicon::PostReply<5>(favicon_base::FaviconRawBitmapResult()));

    no_title_entry_url_ = GURL("http://chromium.org/unread3");
    // The first 3 have the same update time on purpose.
    model_->AddEntry(GURL("http://chromium.org/unread1"), "unread1",
                     reading_list::ADDED_VIA_CURRENT_APP);
    model_->AddEntry(GURL("http://chromium.org/read1"), "read1",
                     reading_list::ADDED_VIA_CURRENT_APP);
    model_->SetReadStatus(GURL("http://chromium.org/read1"), true);
    model_->AddEntry(GURL("http://chromium.org/unread2"), "unread2",
                     reading_list::ADDED_VIA_CURRENT_APP);
    clock_.Advance(base::TimeDelta::FromMilliseconds(10));
    model_->AddEntry(no_title_entry_url_, "",
                     reading_list::ADDED_VIA_CURRENT_APP);
    clock_.Advance(base::TimeDelta::FromMilliseconds(10));
    model_->AddEntry(GURL("http://chromium.org/read2"), "read2",
                     reading_list::ADDED_VIA_CURRENT_APP);
    model_->SetReadStatus(GURL("http://chromium.org/read2"), true);

    large_icon_service_.reset(new favicon::LargeIconService(
        &mock_favicon_service_, /*image_fetcher=*/nullptr));

    mediator_ =
        [[ReadingListMediator alloc] initWithModel:model_.get()
                                  largeIconService:large_icon_service_.get()];
  }

 protected:
  testing::StrictMock<favicon::MockFaviconService> mock_favicon_service_;
  std::unique_ptr<ReadingListModelImpl> model_;
  ReadingListMediator* mediator_;
  base::SimpleTestClock clock_;
  GURL no_title_entry_url_;
  std::unique_ptr<favicon::LargeIconService> large_icon_service_;

 private:
  web::TestWebThreadBundle thread_bundle_;
  DISALLOW_COPY_AND_ASSIGN(ReadingListMediatorTest);
};

TEST_F(ReadingListMediatorTest, fillItems) {
  // Setup.
  NSMutableArray<CollectionViewItem*>* readArray = [NSMutableArray array];
  NSMutableArray<CollectionViewItem*>* unreadArray = [NSMutableArray array];
  id mockDelegate = OCMProtocolMock(
      @protocol(ReadingListCollectionViewItemAccessibilityDelegate));

  // Action.
  [mediator_ fillReadItems:readArray
               unreadItems:unreadArray
              withDelegate:mockDelegate];

  // Tests.
  EXPECT_EQ(3U, [unreadArray count]);
  EXPECT_EQ(2U, [readArray count]);
  NSArray<ReadingListCollectionViewItem*>* rlReadArray = [readArray copy];
  NSArray<ReadingListCollectionViewItem*>* rlUneadArray = [unreadArray copy];
  EXPECT_TRUE([rlUneadArray[0].title
      isEqualToString:base::SysUTF16ToNSString(url_formatter::FormatUrl(
                          no_title_entry_url_.GetOrigin()))]);
  EXPECT_TRUE([rlReadArray[0].title isEqualToString:@"read2"]);
  EXPECT_TRUE([rlReadArray[1].title isEqualToString:@"read1"]);
  EXPECT_EQ(mockDelegate, rlReadArray[0].accessibilityDelegate);
  EXPECT_EQ(mockDelegate, rlReadArray[1].accessibilityDelegate);
  EXPECT_EQ(mockDelegate, rlUneadArray[0].accessibilityDelegate);
  EXPECT_EQ(mockDelegate, rlUneadArray[1].accessibilityDelegate);
  EXPECT_EQ(mockDelegate, rlUneadArray[2].accessibilityDelegate);
}
