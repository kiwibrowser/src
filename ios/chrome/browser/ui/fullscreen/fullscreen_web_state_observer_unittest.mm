// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_web_state_observer.h"

#include "base/test/scoped_feature_list.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"
#import "ios/chrome/browser/ui/fullscreen/test/fullscreen_model_test_util.h"
#import "ios/chrome/browser/ui/fullscreen/test/test_fullscreen_controller.h"
#import "ios/chrome/browser/ui/fullscreen/test/test_fullscreen_mediator.h"
#include "ios/chrome/browser/ui/ui_feature_flags.h"
#import "ios/web/public/navigation_item.h"
#include "ios/web/public/ssl_status.h"
#import "ios/web/public/test/fakes/fake_navigation_context.h"
#import "ios/web/public/test/fakes/test_navigation_manager.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class FullscreenWebStateObserverTest : public PlatformTest {
 public:
  FullscreenWebStateObserverTest()
      : PlatformTest(),
        controller_(&model_),
        mediator_(&controller_, &model_),
        observer_(&controller_, &model_, &mediator_) {
    // Set up a TestNavigationManager.
    std::unique_ptr<web::TestNavigationManager> navigation_manager =
        std::make_unique<web::TestNavigationManager>();
    navigation_manager_ = navigation_manager.get();
    web_state_.SetNavigationManager(std::move(navigation_manager));
    // Begin observing the WebState.
    observer_.SetWebState(&web_state_);
    // Set up model.
    SetUpFullscreenModelForTesting(&model_, 100.0);
  }

  ~FullscreenWebStateObserverTest() override {
    mediator_.Disconnect();
    observer_.SetWebState(nullptr);
  }

  FullscreenModel& model() { return model_; }
  web::TestWebState& web_state() { return web_state_; }
  web::TestNavigationManager& navigation_manager() {
    return *navigation_manager_;
  }

 private:
  FullscreenModel model_;
  TestFullscreenController controller_;
  TestFullscreenMediator mediator_;
  web::TestWebState web_state_;
  web::TestNavigationManager* navigation_manager_;
  FullscreenWebStateObserver observer_;
};

// Tests that the model is reset when a navigation is committed.
TEST_F(FullscreenWebStateObserverTest, ResetForNavigation) {
  // Simulate a scroll to 0.5 progress.
  SimulateFullscreenUserScrollForProgress(&model(), 0.5);
  EXPECT_EQ(model().progress(), 0.5);
  // Simulate a navigation.
  web::FakeNavigationContext context;
  web_state().OnNavigationFinished(&context);
  EXPECT_FALSE(model().has_base_offset());
  EXPECT_EQ(model().progress(), 1.0);
}

// Tests that the FullscreenModel is not reset for a same-document navigation.
TEST_F(FullscreenWebStateObserverTest, NoResetForSameDocument) {
  // Simulate a scroll to 0.5 progress.
  SimulateFullscreenUserScrollForProgress(&model(), 0.5);
  EXPECT_EQ(model().progress(), 0.5);
  // Simulate a same-document navigation and verify that the 0.5 progress hasn't
  // been reset to 1.0.
  web::FakeNavigationContext context;
  context.SetIsSameDocument(true);
  web_state().OnNavigationFinished(&context);
  EXPECT_EQ(model().progress(), 0.5);
}

// Tests that the model is disabled when a load is occurring.
TEST_F(FullscreenWebStateObserverTest, DisableDuringLoadWithUIRefreshDisabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUIRefreshPhase1);

  EXPECT_TRUE(model().enabled());
  web_state().SetLoading(true);
  EXPECT_FALSE(model().enabled());
  web_state().SetLoading(false);
  EXPECT_TRUE(model().enabled());
}

// Tests that the model is not disabled when a load is occurring.
TEST_F(FullscreenWebStateObserverTest, DisableDuringLoadWithUIRefreshEnabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kUIRefreshPhase1);

  EXPECT_TRUE(model().enabled());
  web_state().SetLoading(true);
  EXPECT_TRUE(model().enabled());
  web_state().SetLoading(false);
  EXPECT_TRUE(model().enabled());
}

// Tests that the model is disabled when the SSL status is broken.
TEST_F(FullscreenWebStateObserverTest, DisableForBrokenSSL) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUIRefreshPhase1);

  std::unique_ptr<web::NavigationItem> item = web::NavigationItem::Create();
  item->GetSSL().security_style = web::SECURITY_STYLE_AUTHENTICATION_BROKEN;
  navigation_manager().SetVisibleItem(item.get());
  EXPECT_TRUE(model().enabled());
  web_state().OnVisibleSecurityStateChanged();
  EXPECT_FALSE(model().enabled());
  navigation_manager().SetVisibleItem(nullptr);
  web_state().OnVisibleSecurityStateChanged();
  EXPECT_TRUE(model().enabled());
}

// Tests that the model remains enabled when the SSL status is broken and the
// UI refresh flag is enabled.
TEST_F(FullscreenWebStateObserverTest,
       DisableForBrokenSSLWithUIRefreshEnabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kUIRefreshPhase1);

  std::unique_ptr<web::NavigationItem> item = web::NavigationItem::Create();
  item->GetSSL().security_style = web::SECURITY_STYLE_AUTHENTICATION_BROKEN;
  navigation_manager().SetVisibleItem(item.get());
  EXPECT_TRUE(model().enabled());
  web_state().OnVisibleSecurityStateChanged();
  EXPECT_TRUE(model().enabled());
  navigation_manager().SetVisibleItem(nullptr);
  web_state().OnVisibleSecurityStateChanged();
  EXPECT_TRUE(model().enabled());
}
