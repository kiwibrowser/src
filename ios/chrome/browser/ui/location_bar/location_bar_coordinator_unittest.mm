// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/location_bar/location_bar_legacy_coordinator.h"

#include <memory>

#include "components/toolbar/test_toolbar_model.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_coordinator_delegate.h"
#include "ios/chrome/browser/web_state_list/fake_web_state_list_delegate.h"
#include "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_opener.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#import "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TestToolbarCoordinatorDelegate : NSObject<ToolbarCoordinatorDelegate>

@end

@implementation TestToolbarCoordinatorDelegate {
  std::unique_ptr<ToolbarModel> _model;
}

- (void)locationBarDidBecomeFirstResponder {
}
- (void)locationBarDidResignFirstResponder {
}
- (void)locationBarBeganEdit {
}

- (ToolbarModel*)toolbarModel {
  if (!_model) {
    _model = std::make_unique<TestToolbarModel>();
  }

  return _model.get();
}

- (BOOL)shouldDisplayHintText {
  return NO;
}

@end

namespace {

class LocationBarLegacyCoordinatorTest : public PlatformTest {
 protected:
  LocationBarLegacyCoordinatorTest()
      : web_state_list_(&web_state_list_delegate_) {}

  void SetUp() override {
    PlatformTest::SetUp();

    TestChromeBrowserState::Builder test_cbs_builder;
    browser_state_ = test_cbs_builder.Build();

    auto web_state = std::make_unique<web::TestWebState>();
    web_state->SetCurrentURL(GURL("http://test/"));
    web_state_list_.InsertWebState(0, std::move(web_state),
                                   WebStateList::INSERT_FORCE_INDEX,
                                   WebStateOpener());

    delegate_ = [[TestToolbarCoordinatorDelegate alloc] init];

    coordinator_ = [[LocationBarLegacyCoordinator alloc] init];
    coordinator_.browserState = browser_state_.get();
    coordinator_.webStateList = &web_state_list_;
    coordinator_.delegate = delegate_;
  }

  web::TestWebThreadBundle web_thread_bundle_;
  LocationBarLegacyCoordinator* coordinator_;
  std::unique_ptr<TestChromeBrowserState> browser_state_;
  FakeWebStateListDelegate web_state_list_delegate_;
  WebStateList web_state_list_;
  TestToolbarCoordinatorDelegate* delegate_;
};

TEST_F(LocationBarLegacyCoordinatorTest, Stops) {
  EXPECT_TRUE(coordinator_.view == nil);
  [coordinator_ start];
  EXPECT_TRUE(coordinator_.view != nil);
  [coordinator_ stop];
  EXPECT_TRUE(coordinator_.view == nil);
}

}  // namespace
