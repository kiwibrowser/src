// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_QUEUE_MANAGER_OBSERVER_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_QUEUE_MANAGER_OBSERVER_H_

#include "base/macros.h"

class OverlayQueue;
class OverlayQueueManager;

// Observer class for OverlayQueueManager.
class OverlayQueueManagerObserver {
 public:
  OverlayQueueManagerObserver() = default;
  virtual ~OverlayQueueManagerObserver() = default;

  // Called when an OverlayQueueManager creates |queue|.
  virtual void OverlayQueueManagerDidAddQueue(OverlayQueueManager* manager,
                                              OverlayQueue* queue) {}

  // Called when an OverlayQueueManager is about to remove |queue|.
  virtual void OverlayQueueManagerWillRemoveQueue(OverlayQueueManager* manager,
                                                  OverlayQueue* queue) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(OverlayQueueManagerObserver);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_QUEUE_MANAGER_OBSERVER_H_
