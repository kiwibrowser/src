// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/test/test_overlay_queue_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void TestOverlayQueueObserver::OverlayQueueDidAddOverlay(OverlayQueue* queue) {
  did_add_called_ = true;
}

void TestOverlayQueueObserver::OverlayQueueWillReplaceVisibleOverlay(
    OverlayQueue* queue) {
  will_replace_called_ = true;
}

void TestOverlayQueueObserver::OverlayQueueDidStopVisibleOverlay(
    OverlayQueue* queue) {
  stop_visible_called_ = true;
}

void TestOverlayQueueObserver::OverlayQueueDidCancelOverlays(
    OverlayQueue* queue) {
  did_cancel_called_ = true;
}
