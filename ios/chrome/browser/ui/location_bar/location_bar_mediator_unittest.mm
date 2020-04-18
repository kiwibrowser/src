// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/location_bar/location_bar_legacy_mediator.h"

#include <memory>

#import "ios/chrome/browser/ui/location_bar/location_bar_legacy_consumer.h"
#import "ios/chrome/browser/ui/toolbar/test/toolbar_test_navigation_manager.h"
#import "ios/chrome/browser/ui/toolbar/test/toolbar_test_web_state.h"
#include "ios/chrome/browser/web_state_list/fake_web_state_list_delegate.h"
#include "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_list_observer_bridge.h"
#import "ios/chrome/browser/web_state_list/web_state_opener.h"
#import "ios/web/public/test/fakes/fake_navigation_context.h"
#import "ios/web/public/test/fakes/test_navigation_manager.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#import "ios/web/public/web_state/web_state_observer_bridge.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
static const char kTestUrl[] = "http://www.chromium.org";
static const int kNumberOfWebStates = 3;
}  // namespace

class LocationBarMediatorTest : public PlatformTest {
 public:
  LocationBarMediatorTest() {
    mediator_ = [[LocationBarLegacyMediator alloc] init];
    consumer_ = OCMProtocolMock(@protocol(LocationBarLegacyConsumer));
    consumer_strict_ = OCMProtocolMock(@protocol(LocationBarLegacyConsumer));
    SetUpWebStateList();
  }

  // Explicitly disconnect the mediator so there won't be any WebStateList
  // observers when web_state_list_ gets dealloc.
  ~LocationBarMediatorTest() override { [mediator_ disconnect]; }

 protected:
  void SetUpWebStateList() {
    auto navigation_manager = std::make_unique<ToolbarTestNavigationManager>();
    navigation_manager_ = navigation_manager.get();

    std::unique_ptr<ToolbarTestWebState> test_web_state =
        std::make_unique<ToolbarTestWebState>();
    test_web_state->SetNavigationManager(std::move(navigation_manager));
    test_web_state->SetLoading(true);
    web_state_ = test_web_state.get();

    web_state_list_ = std::make_unique<WebStateList>(&web_state_list_delegate_);
    web_state_list_->InsertWebState(0, std::move(test_web_state),
                                    WebStateList::INSERT_FORCE_INDEX,
                                    WebStateOpener());
    for (int i = 1; i < kNumberOfWebStates; i++) {
      InsertNewWebState(i);
    }
  }

  void InsertNewWebState(int index) {
    auto web_state = std::make_unique<web::TestWebState>();
    GURL url("http://test/" + std::to_string(index));
    web_state->SetCurrentURL(url);
    web_state_list_->InsertWebState(index, std::move(web_state),
                                    WebStateList::INSERT_FORCE_INDEX,
                                    WebStateOpener());
  }

  void SetUpActiveWebState() { web_state_list_->ActivateWebStateAt(0); }

  LocationBarLegacyMediator* mediator_;
  ToolbarTestWebState* web_state_;
  ToolbarTestNavigationManager* navigation_manager_;
  std::unique_ptr<WebStateList> web_state_list_;
  FakeWebStateListDelegate web_state_list_delegate_;
  id consumer_;
  id consumer_strict_;
};

// Test no setup is being done on the LocationBar if there's no Webstate.
TEST_F(LocationBarMediatorTest, TestLocationBarSetupWithNoWebstate) {
  mediator_.consumer = consumer_strict_;
}

// Test no setup is being done on the LocationBar if there's no active Webstate.
TEST_F(LocationBarMediatorTest, TestLocationBarSetupWithNoActiveWebstate) {
  mediator_.webStateList = web_state_list_.get();
  mediator_.consumer = consumer_strict_;
}

// Test no WebstateList related setup is being done on the LocationBar if
// there's no WebstateList.
TEST_F(LocationBarMediatorTest, TestLocationBarSetupWithNoWebstateList) {
  mediator_.consumer = consumer_strict_;
}

// Test the LocationBar Setup gets called when the mediator's WebState and
// Consumer have been set.
TEST_F(LocationBarMediatorTest, TestLocationBarSetup) {
  mediator_.webStateList = web_state_list_.get();
  SetUpActiveWebState();
  OCMExpect([consumer_ updateOmniboxState]);

  mediator_.consumer = consumer_;

  EXPECT_OCMOCK_VERIFY(consumer_);
}

// Test the LocationBar Setup gets called when the mediator's WebState and
// Consumer have been set in reverse order.
TEST_F(LocationBarMediatorTest, TestLocationBarSetupReverse) {
  OCMExpect([consumer_ updateOmniboxState]);
  mediator_.consumer = consumer_;
  mediator_.webStateList = web_state_list_.get();

  SetUpActiveWebState();

  EXPECT_OCMOCK_VERIFY(consumer_);
}

// Test the LocationBar is updated when the Webstate observer method
// DidStartLoading is triggered by SetLoading.
TEST_F(LocationBarMediatorTest, TestDidStartLoading) {
  // Change the default loading state to false to verify the Webstate
  // callback with true.
  web_state_->SetLoading(false);
  mediator_.webStateList = web_state_list_.get();
  SetUpActiveWebState();
  mediator_.consumer = consumer_;

  OCMExpect([consumer_ updateOmniboxState]);

  web_state_->SetLoading(true);

  EXPECT_OCMOCK_VERIFY(consumer_);
}

// Test the LocationBar is updated when the Webstate observer method
// DidStopLoading is triggered by SetLoading.
TEST_F(LocationBarMediatorTest, TestDidStopLoading) {
  mediator_.webStateList = web_state_list_.get();
  SetUpActiveWebState();
  mediator_.consumer = consumer_;

  OCMExpect([consumer_ updateOmniboxState]);

  web_state_->SetLoading(false);

  EXPECT_OCMOCK_VERIFY(consumer_);
}

// Test the LocationBar is updated when the Webstate observer method
// DidLoadPageWithSuccess is triggered by OnPageLoaded.
TEST_F(LocationBarMediatorTest, TestDidLoadPageWithSucess) {
  mediator_.webStateList = web_state_list_.get();
  SetUpActiveWebState();
  mediator_.consumer = consumer_;

  navigation_manager_->set_can_go_forward(true);
  navigation_manager_->set_can_go_back(true);

  web_state_->SetCurrentURL(GURL(kTestUrl));

  OCMExpect([consumer_ updateOmniboxState]);

  web_state_->OnPageLoaded(web::PageLoadCompletionStatus::SUCCESS);

  EXPECT_OCMOCK_VERIFY(consumer_);
}

// Test the LocationBar is updated when the Webstate observer method
// didFinishNavigation is called.
TEST_F(LocationBarMediatorTest, TestDidFinishNavigation) {
  mediator_.webStateList = web_state_list_.get();
  SetUpActiveWebState();
  mediator_.consumer = consumer_;

  navigation_manager_->set_can_go_forward(true);
  navigation_manager_->set_can_go_back(true);

  web_state_->SetCurrentURL(GURL(kTestUrl));
  web::FakeNavigationContext context;

  OCMExpect([consumer_ updateOmniboxState]);

  web_state_->OnNavigationFinished(&context);

  EXPECT_OCMOCK_VERIFY(consumer_);
}

// Test the LocationBar is updated when the Webstate observer method
// didFinishNavigation is called.
TEST_F(LocationBarMediatorTest, TestDidChangeVisibleSecurityState) {
  mediator_.webStateList = web_state_list_.get();
  SetUpActiveWebState();
  mediator_.consumer = consumer_;

  navigation_manager_->set_can_go_forward(true);
  navigation_manager_->set_can_go_back(true);

  web_state_->SetCurrentURL(GURL(kTestUrl));

  OCMExpect([consumer_ updateOmniboxState]);

  web_state_->OnVisibleSecurityStateChanged();

  EXPECT_OCMOCK_VERIFY(consumer_);
}

// Test the omnibox is defocused when the active webstate is changed.
TEST_F(LocationBarMediatorTest, TestChangeActiveWebState) {
  mediator_.webStateList = web_state_list_.get();
  SetUpActiveWebState();
  mediator_.consumer = consumer_;

  OCMExpect([consumer_ defocusOmnibox]);
  web_state_list_->ActivateWebStateAt(1);

  EXPECT_OCMOCK_VERIFY(consumer_);
}
