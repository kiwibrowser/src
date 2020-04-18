// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/browser_overlay_queue.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"
#import "ios/chrome/browser/ui/overlays/overlay_coordinator+internal.h"
#import "ios/chrome/browser/ui/overlays/overlay_coordinator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_BROWSER_USER_DATA_KEY(BrowserOverlayQueue);

BrowserOverlayQueue::BrowserOverlayQueue(Browser* browser)
    : overlay_parents_([[NSMutableArray alloc] init]) {}

BrowserOverlayQueue::~BrowserOverlayQueue() {}

void BrowserOverlayQueue::StartNextOverlay() {
  DCHECK(HasQueuedOverlays());
  DCHECK_EQ(GetCount(), [overlay_parents_ count]);
  DCHECK(!IsShowingOverlay());
  [GetFirstOverlay() startOverlayingCoordinator:[overlay_parents_ firstObject]];
  OverlayWasStarted();
}

void BrowserOverlayQueue::ReplaceVisibleOverlay(
    OverlayCoordinator* overlay_coordinator) {
  DCHECK(IsShowingOverlay());
  // Prepend the current overlay's parent to |overlay_parents_| so that the
  // replacement overlay can also use that as its parents.
  [overlay_parents_ addObject:[overlay_parents_ firstObject]];
  OverlayQueue::ReplaceVisibleOverlay(overlay_coordinator);
}

void BrowserOverlayQueue::OverlayWasStopped(
    OverlayCoordinator* overlay_coordinator) {
  DCHECK_EQ(GetFirstOverlay(), overlay_coordinator);
  [overlay_parents_ removeObjectAtIndex:0];
  OverlayQueue::OverlayWasStopped(overlay_coordinator);
}

void BrowserOverlayQueue::AddBrowserOverlay(
    OverlayCoordinator* overlay_coordinator,
    BrowserCoordinator* overlay_parent) {
  DCHECK(overlay_coordinator);
  DCHECK(overlay_parent);
  [overlay_parents_ addObject:overlay_parent];
  OverlayQueue::AddOverlay(overlay_coordinator);
}
