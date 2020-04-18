// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/overlay_scheduler.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator_test.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator_test_util.h"
#import "ios/chrome/browser/ui/overlays/browser_overlay_queue.h"
#import "ios/chrome/browser/ui/overlays/overlay_queue_manager.h"
#import "ios/chrome/browser/ui/overlays/overlay_scheduler_observer.h"
#import "ios/chrome/browser/ui/overlays/test/test_overlay_coordinator.h"
#import "ios/chrome/browser/ui/overlays/test/test_overlay_parent_coordinator.h"
#import "ios/chrome/browser/ui/overlays/test/test_overlay_queue.h"
#import "ios/chrome/browser/ui/overlays/test/test_overlay_queue_observer.h"
#import "ios/chrome/browser/ui/overlays/web_state_overlay_queue.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_opener.h"
#include "ios/web/public/test/fakes/test_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test observer for the scheduler.
class TestOverlaySchedulerObserver : public OverlaySchedulerObserver {
 public:
  TestOverlaySchedulerObserver(WebStateList* web_state_list)
      : web_state_list_(web_state_list),
        active_index_(WebStateList::kInvalidIndex) {}

  void OverlaySchedulerWillShowOverlay(OverlayScheduler* scheduler,
                                       web::WebState* web_state) override {
    active_index_ = web_state_list_->GetIndexOfWebState(web_state);
    if (web_state) {
      EXPECT_NE(active_index_, WebStateList::kInvalidIndex);
      web_state_list_->ActivateWebStateAt(active_index_);
      web_state->WasShown();
    }
  }

  int active_index() { return active_index_; }

 private:
  WebStateList* web_state_list_;
  int active_index_;
};

// Test fixture for OverlayScheduler.
class OverlaySchedulerTest : public BrowserCoordinatorTest {
 public:
  OverlaySchedulerTest() {
    observer_ =
        std::make_unique<TestOverlaySchedulerObserver>(web_state_list());

    OverlayQueueManager::CreateForBrowser(GetBrowser());
    OverlayScheduler::CreateForBrowser(GetBrowser());
    scheduler()->SetQueueManager(manager());
    scheduler()->AddObserver(observer());
  }

  ~OverlaySchedulerTest() override {
    scheduler()->RemoveObserver(observer());
    scheduler()->Disconnect();
  }

  WebStateList* web_state_list() { return &(GetBrowser()->web_state_list()); }

  TestOverlaySchedulerObserver* observer() { return observer_.get(); }
  OverlayQueueManager* manager() {
    return OverlayQueueManager::FromBrowser(GetBrowser());
  }
  OverlayScheduler* scheduler() {
    return OverlayScheduler::FromBrowser(GetBrowser());
  }

 private:
  std::unique_ptr<TestOverlaySchedulerObserver> observer_;
  TestOverlayQueue queue_;

  DISALLOW_COPY_AND_ASSIGN(OverlaySchedulerTest);
};

// Tests that adding an overlay to the BrowserOverlayQueue triggers successful
// presentation.
TEST_F(OverlaySchedulerTest, AddBrowserOverlay) {
  BrowserOverlayQueue* queue = BrowserOverlayQueue::FromBrowser(GetBrowser());
  ASSERT_TRUE(queue);
  TestOverlayParentCoordinator* parent =
      [[TestOverlayParentCoordinator alloc] init];
  TestOverlayCoordinator* overlay = [[TestOverlayCoordinator alloc] init];
  queue->AddBrowserOverlay(overlay, parent);
  EXPECT_EQ(parent.presentedOverlay, overlay);
  WaitForBrowserCoordinatorActivation(overlay);
  EXPECT_TRUE(scheduler()->IsShowingOverlay());
  [overlay stop];
  WaitForBrowserCoordinatorDeactivation(overlay);
  EXPECT_EQ(parent.presentedOverlay, nil);
  EXPECT_FALSE(scheduler()->IsShowingOverlay());
}

// Tests that adding an overlay to a WebStateOverlayQueue triggers successful
// presentation.
TEST_F(OverlaySchedulerTest, AddWebStateOverlay) {
  web_state_list()->InsertWebState(0, std::make_unique<web::TestWebState>(),
                                   WebStateList::INSERT_FORCE_INDEX,
                                   WebStateOpener());
  web_state_list()->ActivateWebStateAt(0);
  ASSERT_EQ(web_state_list()->count(), 1);
  web::WebState* web_state = web_state_list()->GetWebStateAt(0);
  WebStateOverlayQueue* queue = WebStateOverlayQueue::FromWebState(web_state);
  ASSERT_TRUE(queue);
  TestOverlayParentCoordinator* parent =
      [[TestOverlayParentCoordinator alloc] init];
  queue->SetWebStateParentCoordinator(parent);
  TestOverlayCoordinator* overlay = [[TestOverlayCoordinator alloc] init];
  queue->AddWebStateOverlay(overlay);
  EXPECT_EQ(parent.presentedOverlay, overlay);
  WaitForBrowserCoordinatorActivation(overlay);
  EXPECT_TRUE(scheduler()->IsShowingOverlay());
  [overlay stop];
  WaitForBrowserCoordinatorDeactivation(overlay);
  EXPECT_EQ(parent.presentedOverlay, nil);
  EXPECT_FALSE(scheduler()->IsShowingOverlay());
}

// Tests that attempting to present an overlay for an inactive WebState will
// correctly switch the active WebState before presenting.
TEST_F(OverlaySchedulerTest, SwitchWebStateForOverlay) {
  // Add two WebStates and activate the second.
  web_state_list()->InsertWebState(0, std::make_unique<web::TestWebState>(),
                                   WebStateList::INSERT_FORCE_INDEX,
                                   WebStateOpener());
  ASSERT_EQ(web_state_list()->count(), 1);
  web_state_list()->InsertWebState(1, std::make_unique<web::TestWebState>(),
                                   WebStateList::INSERT_FORCE_INDEX,
                                   WebStateOpener());
  web_state_list()->ActivateWebStateAt(1);
  // Add an overlay to the queue corresponding with the first WebState.
  WebStateOverlayQueue* queue =
      WebStateOverlayQueue::FromWebState(web_state_list()->GetWebStateAt(0));
  ASSERT_TRUE(queue);
  TestOverlayParentCoordinator* parent =
      [[TestOverlayParentCoordinator alloc] init];
  queue->SetWebStateParentCoordinator(parent);
  TestOverlayCoordinator* overlay = [[TestOverlayCoordinator alloc] init];
  queue->AddWebStateOverlay(overlay);
  // Verify that the overlay is presented and the active WebState index is
  // updated.
  EXPECT_EQ(parent.presentedOverlay, overlay);
  WaitForBrowserCoordinatorActivation(overlay);
  EXPECT_TRUE(scheduler()->IsShowingOverlay());
  EXPECT_EQ(observer()->active_index(), 0);
  [overlay stop];
  WaitForBrowserCoordinatorDeactivation(overlay);
  EXPECT_EQ(parent.presentedOverlay, nil);
  EXPECT_FALSE(scheduler()->IsShowingOverlay());
}

// Tests that attempting to present an overlay for an inactive WebState will
// correctly switch the active WebState before presenting.
TEST_F(OverlaySchedulerTest, SwitchWebStateForQueuedOverlays) {
  // Add two WebStates and activate the second.
  web_state_list()->InsertWebState(0, std::make_unique<web::TestWebState>(),
                                   WebStateList::INSERT_FORCE_INDEX,
                                   WebStateOpener());
  ASSERT_EQ(web_state_list()->count(), 1);
  web_state_list()->InsertWebState(1, std::make_unique<web::TestWebState>(),
                                   WebStateList::INSERT_FORCE_INDEX,
                                   WebStateOpener());
  web_state_list()->ActivateWebStateAt(1);
  // Add an overlay to the queue corresponding with the first WebState.
  WebStateOverlayQueue* queue_0 =
      WebStateOverlayQueue::FromWebState(web_state_list()->GetWebStateAt(0));
  ASSERT_TRUE(queue_0);
  TestOverlayParentCoordinator* parent_0 =
      [[TestOverlayParentCoordinator alloc] init];
  queue_0->SetWebStateParentCoordinator(parent_0);
  TestOverlayCoordinator* overlay_0 = [[TestOverlayCoordinator alloc] init];
  queue_0->AddWebStateOverlay(overlay_0);
  EXPECT_EQ(parent_0.presentedOverlay, overlay_0);
  WaitForBrowserCoordinatorActivation(overlay_0);
  EXPECT_TRUE(scheduler()->IsShowingOverlay());
  EXPECT_EQ(observer()->active_index(), 0);
  // Add an overlay to the queue corresponding with the second WebState.
  WebStateOverlayQueue* queue_1 =
      WebStateOverlayQueue::FromWebState(web_state_list()->GetWebStateAt(1));
  ASSERT_TRUE(queue_1);
  TestOverlayParentCoordinator* parent_1 =
      [[TestOverlayParentCoordinator alloc] init];
  queue_1->SetWebStateParentCoordinator(parent_1);
  TestOverlayCoordinator* overlay_1 = [[TestOverlayCoordinator alloc] init];
  queue_1->AddWebStateOverlay(overlay_1);
  // Stop the first overlay and verify that the second overlay has been
  // presented and the active index updated.
  [overlay_0 stop];
  WaitForBrowserCoordinatorDeactivation(overlay_0);
  EXPECT_EQ(parent_1.presentedOverlay, overlay_1);
  WaitForBrowserCoordinatorActivation(overlay_1);
  EXPECT_TRUE(scheduler()->IsShowingOverlay());
  EXPECT_EQ(observer()->active_index(), 1);
  EXPECT_EQ(parent_0.presentedOverlay, nil);
  [overlay_1 stop];
  WaitForBrowserCoordinatorDeactivation(overlay_1);
  EXPECT_EQ(parent_1.presentedOverlay, nil);
  EXPECT_FALSE(scheduler()->IsShowingOverlay());
}

// Tests that pausing the scheduler prevents overlays from being started and
// that unpausing the scheduler successfully shows queued overlays.
TEST_F(OverlaySchedulerTest, PauseUnpause) {
  scheduler()->SetPaused(true);
  BrowserOverlayQueue* queue = BrowserOverlayQueue::FromBrowser(GetBrowser());
  ASSERT_TRUE(queue);
  TestOverlayParentCoordinator* parent =
      [[TestOverlayParentCoordinator alloc] init];
  TestOverlayCoordinator* overlay = [[TestOverlayCoordinator alloc] init];
  queue->AddBrowserOverlay(overlay, parent);
  EXPECT_FALSE(parent.presentedOverlay);
  EXPECT_FALSE(scheduler()->IsShowingOverlay());
  scheduler()->SetPaused(false);
  EXPECT_EQ(parent.presentedOverlay, overlay);
  WaitForBrowserCoordinatorActivation(overlay);
  EXPECT_TRUE(scheduler()->IsShowingOverlay());
  [overlay stop];
}
