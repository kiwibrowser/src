// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_BROWSER_OVERLAY_QUEUE_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_BROWSER_OVERLAY_QUEUE_H_

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/ui/browser_list/browser_user_data.h"
#import "ios/chrome/browser/ui/overlays/overlay_queue.h"

@class BrowserCoordinator;

// Class used to queue overlays to display over the browser content area
// regardless of which WebState's content is currently visible.
class BrowserOverlayQueue : public BrowserUserData<BrowserOverlayQueue>,
                            public OverlayQueue {
 public:
  ~BrowserOverlayQueue() override;

  // OverlayQueue:
  void StartNextOverlay() override;
  void ReplaceVisibleOverlay(OverlayCoordinator* overlay_coordinator) override;
  void OverlayWasStopped(OverlayCoordinator* overlay_coordinator) override;

  // Adds |overlay_coordinator| to the queue.  |overlay_parent| will be used as
  // the parent coordinator when StartNextOverlay() is called.
  void AddBrowserOverlay(OverlayCoordinator* overlay_coordinator,
                         BrowserCoordinator* overlay_parent);

 private:
  friend class BrowserUserData<BrowserOverlayQueue>;

  // Private constructor.
  explicit BrowserOverlayQueue(Browser* browser);

  // The parent coordinators added with each overlay.
  NSMutableArray<BrowserCoordinator*>* overlay_parents_;

  DISALLOW_COPY_AND_ASSIGN(BrowserOverlayQueue);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_BROWSER_OVERLAY_QUEUE_H_
