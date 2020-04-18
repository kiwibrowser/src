// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main/browser_view_wrangler.h"

#import <UIKit/UIKit.h>

#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/browser_view_controller.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class BrowserViewWranglerTest : public PlatformTest {
 protected:
  BrowserViewWranglerTest() {
    TestChromeBrowserState::Builder test_cbs_builder;
    chrome_browser_state_ = test_cbs_builder.Build();
  }

  web::TestWebThreadBundle thread_bundle_;
  std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
};

TEST_F(BrowserViewWranglerTest, TestInitNilObserver) {
  // |thread_bundle_| must outlive all objects created by BVC, because those
  // objects may rely on threading API in dealloc.
  @autoreleasepool {
    BrowserViewWrangler* wrangler = [[BrowserViewWrangler alloc]
              initWithBrowserState:chrome_browser_state_.get()

                  tabModelObserver:nil
        applicationCommandEndpoint:(id<ApplicationCommands>)nil];
    // Test that BVC and tab model are created on demand.
    BrowserViewController* bvc = [wrangler mainBVC];
    EXPECT_NE(bvc, nil);

    TabModel* tabModel = [wrangler mainTabModel];
    EXPECT_NE(tabModel, nil);

    // Test that once created the BVC and tab model aren't re-created.
    EXPECT_EQ(bvc, [wrangler mainBVC]);
    EXPECT_EQ(tabModel, [wrangler mainTabModel]);

    // Test that the OTR objects are (a) OTR and (b) not the same as the non-OTR
    // objects.
    EXPECT_NE(bvc, [wrangler otrBVC]);
    EXPECT_NE(tabModel, [wrangler otrTabModel]);
    EXPECT_TRUE([wrangler otrTabModel].browserState->IsOffTheRecord());

    [wrangler shutdown];
  }
}

}  // namespace
