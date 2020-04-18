// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_TEST_OVERLAY_QUEUE_OBSERVER_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_TEST_OVERLAY_QUEUE_OBSERVER_H_

#import "ios/chrome/browser/ui/overlays/overlay_queue_observer.h"

// Test OverlayQueueObserver.
class TestOverlayQueueObserver : public OverlayQueueObserver {
 public:
  TestOverlayQueueObserver()
      : did_add_called_(false),
        will_replace_called_(false),
        stop_visible_called_(false),
        did_cancel_called_(false) {}

  // OverlayQueueObserver:
  void OverlayQueueDidAddOverlay(OverlayQueue* queue) override;
  void OverlayQueueWillReplaceVisibleOverlay(OverlayQueue* queue) override;
  void OverlayQueueDidStopVisibleOverlay(OverlayQueue* queue) override;
  void OverlayQueueDidCancelOverlays(OverlayQueue* queue) override;

  // Accessors for flags.
  bool did_add_called() { return did_add_called_; }
  bool will_replace_called() { return will_replace_called_; }
  bool stop_visible_called() { return stop_visible_called_; }
  bool did_cancel_called() { return did_cancel_called_; }

 private:
  // Flags denoting which observer callbacks have been called.
  bool did_add_called_;
  bool will_replace_called_;
  bool stop_visible_called_;
  bool did_cancel_called_;

  DISALLOW_COPY_AND_ASSIGN(TestOverlayQueueObserver);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_TEST_OVERLAY_QUEUE_OBSERVER_H_
