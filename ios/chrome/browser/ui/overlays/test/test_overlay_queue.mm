// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/test/test_overlay_queue.h"

#import "ios/chrome/browser/ui/coordinators/browser_coordinator_test_util.h"
#import "ios/chrome/browser/ui/overlays/overlay_coordinator+internal.h"
#import "ios/chrome/browser/ui/overlays/overlay_coordinator.h"
#import "ios/chrome/browser/ui/overlays/test/test_overlay_parent_coordinator.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

TestOverlayQueue::TestOverlayQueue()
    : OverlayQueue(), parent_([[TestOverlayParentCoordinator alloc] init]) {}

void TestOverlayQueue::AddOverlay(OverlayCoordinator* overlay) {
  OverlayQueue::AddOverlay(overlay);
}

void TestOverlayQueue::StartNextOverlay() {
  OverlayCoordinator* overlay = GetFirstOverlay();
  EXPECT_TRUE(overlay);
  [overlay startOverlayingCoordinator:parent_];
  OverlayWasStarted();
  WaitForBrowserCoordinatorActivation(overlay);
}

void TestOverlayQueue::ReplaceVisibleOverlay(
    OverlayCoordinator* overlay_coordinator) {
  OverlayCoordinator* overlay = GetFirstOverlay();
  EXPECT_TRUE(overlay);
  OverlayQueue::ReplaceVisibleOverlay(overlay_coordinator);
  WaitForBrowserCoordinatorDeactivation(overlay);
}

void TestOverlayQueue::SetBrowser(Browser* browser) {
  browser_ = browser;
  parent_.browser = browser;
}
