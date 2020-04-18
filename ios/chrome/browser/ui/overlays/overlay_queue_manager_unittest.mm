// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/overlay_queue_manager.h"

#include <memory>

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator_test.h"
#import "ios/chrome/browser/ui/overlays/overlay_queue.h"
#import "ios/chrome/browser/ui/overlays/test/test_overlay_coordinator.h"
#import "ios/chrome/browser/ui/overlays/test/test_overlay_queue_manager_observer.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_opener.h"
#include "ios/web/public/test/fakes/test_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Returns the OverlayQueue in |queues| that retuns |web_state| for
// GetWebState().
OverlayQueue* GetQueueForWebState(const std::set<OverlayQueue*>& queues,
                                  web::WebState* web_state) {
  auto queue_iter = queues.begin();
  while (queue_iter != queues.end()) {
    if ((*queue_iter)->GetWebState() == web_state)
      break;
    ++queue_iter;
  }
  return queue_iter == queues.end() ? nullptr : *queue_iter;
}
}

class OverlayQueueManagerTest : public BrowserCoordinatorTest {
 public:
  OverlayQueueManagerTest() {
    OverlayQueueManager::CreateForBrowser(GetBrowser());
    manager()->AddObserver(&observer_);
  }

  ~OverlayQueueManagerTest() override {
    manager()->RemoveObserver(&observer_);
    manager()->Disconnect();
  }

  WebStateList* web_state_list() { return &(GetBrowser()->web_state_list()); }

  OverlayQueueManager* manager() {
    return OverlayQueueManager::FromBrowser(GetBrowser());
  }

  TestOverlayQueueManagerObserver* observer() { return &observer_; }

 private:
  TestOverlayQueueManagerObserver observer_;

  DISALLOW_COPY_AND_ASSIGN(OverlayQueueManagerTest);
};

// Tests that an OverlayQueueManager for a Browser with no WebStates contains
// one Browser-specific OverlayQueue.
TEST_F(OverlayQueueManagerTest, NoWebStates) {
  const std::set<OverlayQueue*>& queues = manager()->queues();
  EXPECT_EQ(queues.size(), 1U);
  ASSERT_NE(queues.begin(), queues.end());
  EXPECT_FALSE((*queues.begin())->GetWebState());
}

// Tests that adding and removing a WebState successfully creates queues and
// notifies observers.
TEST_F(OverlayQueueManagerTest, AddAndRemoveWebState) {
  // Create a WebState and add it to the WebStateList, verifying that the
  // manager's queues are updated and that the observer callback is received.
  std::unique_ptr<web::WebState> web_state =
      std::make_unique<web::TestWebState>();
  web_state_list()->InsertWebState(0, std::move(web_state),
                                   WebStateList::INSERT_FORCE_INDEX,
                                   WebStateOpener());
  ASSERT_EQ(web_state_list()->count(), 1);
  web::WebState* inserted_web_state = web_state_list()->GetWebStateAt(0);
  const std::set<OverlayQueue*>& queues = manager()->queues();
  EXPECT_EQ(queues.size(), 2U);
  EXPECT_TRUE(GetQueueForWebState(queues, inserted_web_state));
  EXPECT_TRUE(observer()->did_add_called());
  // Remove the WebState from the WebStateList and verify that its queue is
  // removed and that the observer callback is received.
  web_state_list()->CloseWebStateAt(0, WebStateList::CLOSE_USER_ACTION);
  EXPECT_EQ(queues.size(), 1U);
  EXPECT_FALSE(GetQueueForWebState(queues, inserted_web_state));
  EXPECT_TRUE(observer()->will_remove_called());
}

// Tests that adding and removing a WebState successfully creates queues and
// notifies observers.
TEST_F(OverlayQueueManagerTest, AddAndReplaceWebState) {
  // Create a WebState and add it to the WebStateList, verifying that the
  // manager's queues are updated and that the observer callback is received.
  std::unique_ptr<web::WebState> web_state =
      std::make_unique<web::TestWebState>();
  web_state_list()->InsertWebState(0, std::move(web_state),
                                   WebStateList::INSERT_FORCE_INDEX,
                                   WebStateOpener());
  ASSERT_EQ(web_state_list()->count(), 1);
  web::WebState* inserted_web_state = web_state_list()->GetWebStateAt(0);
  const std::set<OverlayQueue*>& queues = manager()->queues();
  EXPECT_EQ(queues.size(), 2U);
  EXPECT_TRUE(GetQueueForWebState(queues, inserted_web_state));
  EXPECT_TRUE(observer()->did_add_called());
  // Replace |web_state| and verify that the observer callbacks were called and
  // that the queues were successfully added and removed.
  observer()->reset();
  std::unique_ptr<web::WebState> replacement =
      std::make_unique<web::TestWebState>();
  web_state_list()->ReplaceWebStateAt(0, std::move(replacement));
  EXPECT_TRUE(observer()->will_remove_called());
  EXPECT_TRUE(observer()->did_add_called());
  EXPECT_EQ(queues.size(), 2U);
  EXPECT_FALSE(GetQueueForWebState(queues, inserted_web_state));
  inserted_web_state = web_state_list()->GetWebStateAt(0);
  EXPECT_TRUE(GetQueueForWebState(queues, inserted_web_state));
}
