// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_TEST_OVERLAY_QUEUE_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_TEST_OVERLAY_QUEUE_H_

#import "ios/chrome/browser/ui/overlays/overlay_queue.h"

class Browser;
@class BrowserCoordinator;
@class OverlayCoordinator;

// Test OverlayQueue implementation.  This object constructs a dummy parent
// coordinator from which to start queued OverlayCoordinators.
class TestOverlayQueue : public OverlayQueue {
 public:
  TestOverlayQueue();

  // Adds |overlay| to queue to be started over |parent_|.
  void AddOverlay(OverlayCoordinator* overlay);

  // Starts the first overlay in the queue using |parent_| as the parent
  // coordinator.  Waits until the overlay's view controller has finished being
  // presented before returning.
  void StartNextOverlay() override;

  // Replaces the visible overlay with |overlay_coordinator|, waiting until the
  // visible overlay's view controller has finished being dimissed before
  // returning.
  void ReplaceVisibleOverlay(OverlayCoordinator* overlay_coordinator) override;

  // Seting the Browser also sets the Browser of |parent_|, which will be passed
  // along to OverlayCoordinators presented by this queue when they are started.
  Browser* browser() const { return browser_; }
  void SetBrowser(Browser* browser);

 private:
  // The coordinator to use as the parent for overlays added via AddOverlay().
  __strong BrowserCoordinator* parent_;
  // The Browser to use, if any.
  Browser* browser_;

  DISALLOW_COPY_AND_ASSIGN(TestOverlayQueue);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_TEST_OVERLAY_QUEUE_H_
