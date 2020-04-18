// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_HELPERS_OVERLAY_COORDINATOR_TEST_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_HELPERS_OVERLAY_COORDINATOR_TEST_H_

#import "ios/chrome/browser/ui/coordinators/browser_coordinator_test.h"
#import "ios/chrome/browser/ui/overlays/test/test_overlay_queue.h"

@class OverlayCoordinator;

// A test fixture for OverlayCoordinators.
class OverlayCoordinatorTest : public BrowserCoordinatorTest {
 public:
  OverlayCoordinatorTest();
  ~OverlayCoordinatorTest() override;

  // Starts the OverlayCoordinator supplied by GetOverlay().
  void StartOverlay();

 protected:
  // Returns the OverlayCoordinator being tested by this fixture.
  virtual OverlayCoordinator* GetOverlay() = 0;

 private:
  // The queue to handle displying the overlay.
  TestOverlayQueue queue_;
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_HELPERS_OVERLAY_COORDINATOR_TEST_H_
