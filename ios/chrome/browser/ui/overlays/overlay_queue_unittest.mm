// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/overlay_queue.h"

#import "ios/chrome/browser/ui/overlays/test/test_overlay_coordinator.h"
#import "ios/chrome/browser/ui/overlays/test/test_overlay_queue.h"
#import "ios/chrome/browser/ui/overlays/test/test_overlay_queue_observer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class OverlayQueueTest : public PlatformTest {
 public:
  OverlayQueueTest() : PlatformTest() { queue_.AddObserver(&observer_); }

  TestOverlayQueue& queue() { return queue_; }
  TestOverlayQueueObserver& observer() { return observer_; }

 private:
  TestOverlayQueue queue_;
  TestOverlayQueueObserver observer_;
};

// Tests that adding an overlay to the queue updates state accordingly and
// notifies observers.
TEST_F(OverlayQueueTest, AddOverlay) {
  OverlayCoordinator* overlay = [[TestOverlayCoordinator alloc] init];
  queue().AddOverlay(overlay);
  EXPECT_TRUE(queue().HasQueuedOverlays());
  EXPECT_FALSE(queue().IsShowingOverlay());
  EXPECT_TRUE(observer().did_add_called());
}

// Tests that OverlayWasStopped() updates state properly.
TEST_F(OverlayQueueTest, OverlayWasStopped) {
  OverlayCoordinator* overlay = [[TestOverlayCoordinator alloc] init];
  queue().AddOverlay(overlay);
  queue().StartNextOverlay();
  queue().OverlayWasStopped(overlay);
  EXPECT_FALSE(queue().HasQueuedOverlays());
  EXPECT_FALSE(queue().IsShowingOverlay());
  EXPECT_TRUE(observer().stop_visible_called());
}

// Tests that the observer is notified when
TEST_F(OverlayQueueTest, ReplaceOverlay) {
  OverlayCoordinator* overlay = [[TestOverlayCoordinator alloc] init];
  queue().AddOverlay(overlay);
  queue().StartNextOverlay();
  OverlayCoordinator* replacement = [[TestOverlayCoordinator alloc] init];
  queue().ReplaceVisibleOverlay(replacement);
  EXPECT_TRUE(observer().will_replace_called());
  EXPECT_TRUE(observer().stop_visible_called());
}

// Tests that CancelOverlays() cancels all queued overlays for a WebState.
TEST_F(OverlayQueueTest, CancelOverlays) {
  TestOverlayCoordinator* overlay0 = [[TestOverlayCoordinator alloc] init];
  TestOverlayCoordinator* overlay1 = [[TestOverlayCoordinator alloc] init];
  queue().AddOverlay(overlay0);
  queue().AddOverlay(overlay1);
  queue().CancelOverlays();
  EXPECT_FALSE(queue().HasQueuedOverlays());
  EXPECT_FALSE(queue().IsShowingOverlay());
  EXPECT_TRUE(observer().did_cancel_called());
  EXPECT_TRUE(overlay0.cancelled);
  EXPECT_TRUE(overlay1.cancelled);
}
