// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_QUEUE_OBSERVER_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_QUEUE_OBSERVER_H_

#include "base/macros.h"

class OverlayQueue;

// Interface for listening to events occurring to OverlayQueues.
class OverlayQueueObserver {
 public:
  OverlayQueueObserver() = default;
  virtual ~OverlayQueueObserver() = default;

  // Tells the observer that an OverlayCoordinator was added to |queue|.
  virtual void OverlayQueueDidAddOverlay(OverlayQueue* queue) {}

  // Tells the observer that the overlay being presented by |queue| is about to
  // be stopped so that a replacement overlay can be started.
  virtual void OverlayQueueWillReplaceVisibleOverlay(OverlayQueue* queue) {}

  // Tells the observer that the overlay being presented by |queue| was stopped.
  virtual void OverlayQueueDidStopVisibleOverlay(OverlayQueue* queue) {}

  // Tells the observer that the overlays in |queue| were cancelled.
  virtual void OverlayQueueDidCancelOverlays(OverlayQueue* queue) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(OverlayQueueObserver);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_QUEUE_OBSERVER_H_
