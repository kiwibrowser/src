// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/legacy_toolbar_ui_updater.h"

#include <memory>

#import "ios/chrome/browser/ui/toolbar/toolbar_owner.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_ui.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/web_state_list/fake_web_state_list_delegate.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_opener.h"
#import "ios/web/public/test/fakes/fake_navigation_context.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TestToolbarOwner : NSObject<ToolbarHeightProviderForFullscreen>
// Define writable property with same name as |-nonFullscreenToolbarHeight|
// getter defined in ToolbarHeightProviderForFullscreen.
@property(nonatomic, assign) CGFloat nonFullscreenToolbarHeight;
@end

@implementation TestToolbarOwner
@synthesize nonFullscreenToolbarHeight = _nonFullscreenToolbarHeight;
@end

class LegacyToolbarUIUpdaterTest : public PlatformTest {
 public:
  LegacyToolbarUIUpdaterTest()
      : PlatformTest(),
        web_state_list_(&web_state_list_delegate_),
        toolbar_owner_([[TestToolbarOwner alloc] init]),
        toolbar_ui_([[ToolbarUIState alloc] init]),
        updater_([[LegacyToolbarUIUpdater alloc]
            initWithToolbarUI:toolbar_ui_
                 toolbarOwner:toolbar_owner_
                 webStateList:&web_state_list_]) {}
  ~LegacyToolbarUIUpdaterTest() override { StopUpdating(); }

  // Getters.
  WebStateList* web_state_list() { return &web_state_list_; }
  TestToolbarOwner* toolbar_owner() { return toolbar_owner_; }
  CGFloat toolbar_height() { return toolbar_ui_.toolbarHeight; }

  // Start or stop updating the state.
  void StartUpdating() {
    if (updating_)
      return;
    [updater_ startUpdating];
    updating_ = true;
  }
  void StopUpdating() {
    if (!updating_)
      return;
    [updater_ stopUpdating];
    updating_ = false;
  }

  // Inserts and activates a new WebState at the end of the list, and returns a
  // pointer to the inserted WebState.
  web::TestWebState* InsertActiveWebState() {
    std::unique_ptr<web::TestWebState> web_state =
        std::make_unique<web::TestWebState>();
    web::TestWebState* inserted_web_state = web_state.get();
    web_state_list_.InsertWebState(0, std::move(web_state),
                                   WebStateList::INSERT_ACTIVATE,
                                   WebStateOpener(nullptr));
    return inserted_web_state;
  }

 private:
  FakeWebStateListDelegate web_state_list_delegate_;
  WebStateList web_state_list_;
  __strong TestToolbarOwner* toolbar_owner_ = nil;
  __strong ToolbarUIState* toolbar_ui_ = nil;
  __strong LegacyToolbarUIUpdater* updater_ = nil;
  bool updating_ = false;
};

// Tests that |-startUpdating| resets the state's height when starting.
TEST_F(LegacyToolbarUIUpdaterTest, StartUpdating) {
  EXPECT_EQ(toolbar_height(), 0.0);
  const CGFloat kHeight = 150.0;
  toolbar_owner().nonFullscreenToolbarHeight = kHeight;
  StartUpdating();
  EXPECT_EQ(toolbar_height(), kHeight);
}

// Tests that the state is not updated after calling |-stopUpdating|.
TEST_F(LegacyToolbarUIUpdaterTest, StopUpdating) {
  web::TestWebState* web_state = InsertActiveWebState();
  StartUpdating();
  const CGFloat kHeight = 150.0;
  toolbar_owner().nonFullscreenToolbarHeight = kHeight;
  web::FakeNavigationContext context;
  web_state->OnNavigationFinished(&context);
  EXPECT_EQ(toolbar_height(), kHeight);
  const CGFloat kNonUpdatedHeight = 500.0;
  StopUpdating();
  toolbar_owner().nonFullscreenToolbarHeight = kNonUpdatedHeight;
  web_state->OnNavigationFinished(&context);
  EXPECT_EQ(toolbar_height(), kHeight);
}

// Tests that the updater polls for the new height when the active WebState
// changes.
TEST_F(LegacyToolbarUIUpdaterTest, UpdateActiveWebState) {
  StartUpdating();
  const CGFloat kHeight = 150.0;
  toolbar_owner().nonFullscreenToolbarHeight = kHeight;
  EXPECT_EQ(toolbar_height(), 0.0);
  InsertActiveWebState();
  EXPECT_EQ(toolbar_height(), kHeight);
}

// Tests that the updater polls for the new height when the active WebState
// starts a user-initiated navigation.
TEST_F(LegacyToolbarUIUpdaterTest, UserInitiatedNavigation) {
  web::TestWebState* web_state = InsertActiveWebState();
  StartUpdating();
  const CGFloat kHeight = 150.0;
  toolbar_owner().nonFullscreenToolbarHeight = kHeight;
  EXPECT_EQ(toolbar_height(), 0.0);
  web::FakeNavigationContext context;
  context.SetIsRendererInitiated(false);
  web_state->OnNavigationStarted(&context);
  EXPECT_EQ(toolbar_height(), kHeight);
}

// Tests that the updater waits until a render-initiated navigation is committed
// before updating the ui state.
TEST_F(LegacyToolbarUIUpdaterTest, RendererInitiatedNavigation) {
  web::TestWebState* web_state = InsertActiveWebState();
  StartUpdating();
  const CGFloat kHeight = 150.0;
  toolbar_owner().nonFullscreenToolbarHeight = kHeight;
  EXPECT_EQ(toolbar_height(), 0.0);
  web::FakeNavigationContext context;
  context.SetIsRendererInitiated(true);
  web_state->OnNavigationStarted(&context);
  EXPECT_EQ(toolbar_height(), 0.0);
  web_state->OnNavigationFinished(&context);
  EXPECT_EQ(toolbar_height(), kHeight);
}
