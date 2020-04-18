// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/chrome_activity_item_thumbnail_generator.h"

#include "base/test/scoped_task_environment.h"
#import "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"
#include "ios/chrome/browser/tabs/tab.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "ui/base/test/ios/ui_image_test_utils.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class ChromeActivityItemThumbnailGeneratorTest : public PlatformTest {
 protected:
  ChromeActivityItemThumbnailGeneratorTest() {
    chrome_browser_state_ = TestChromeBrowserState::Builder().Build();

    CGRect frame = {CGPointZero, CGSizeMake(400, 300)};
    UIView* view = [[UIView alloc] initWithFrame:frame];
    view.backgroundColor = [UIColor redColor];
    test_web_state_.SetView(view);

    SnapshotTabHelper::CreateForWebState(&test_web_state_,
                                         [[NSUUID UUID] UUIDString]);
  }

  Tab* CreateMockTabForThumbnail(bool incognito) {
    test_web_state_.SetBrowserState(
        incognito ? chrome_browser_state_->GetOffTheRecordChromeBrowserState()
                  : chrome_browser_state_.get());

    web::WebState* web_state = &test_web_state_;
    id tab = [OCMockObject niceMockForClass:[Tab class]];
    OCMStub([tab webState]).andReturn(web_state);
    return tab;
  }

  base::test::ScopedTaskEnvironment task_environment_;
  std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
  web::TestWebState test_web_state_;
};

TEST_F(ChromeActivityItemThumbnailGeneratorTest, ThumbnailForNonIncognitoTab) {
  Tab* tab = CreateMockTabForThumbnail(/*incognito=*/false);

  CGSize size = CGSizeMake(50, 50);
  ThumbnailGeneratorBlock generatorBlock =
      activity_services::ThumbnailGeneratorForTab(tab);
  EXPECT_TRUE(generatorBlock);
  UIImage* thumbnail = generatorBlock(size);
  EXPECT_TRUE(thumbnail);
  EXPECT_TRUE(CGSizeEqualToSize(thumbnail.size, size));
}

TEST_F(ChromeActivityItemThumbnailGeneratorTest, NoThumbnailForIncognitoTab) {
  Tab* tab = CreateMockTabForThumbnail(/*incognito=*/true);

  CGSize size = CGSizeMake(50, 50);
  ThumbnailGeneratorBlock generatorBlock =
      activity_services::ThumbnailGeneratorForTab(tab);
  EXPECT_TRUE(generatorBlock);
  UIImage* thumbnail = generatorBlock(size);
  EXPECT_FALSE(thumbnail);
}

}  // namespace
