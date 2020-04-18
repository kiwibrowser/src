// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/overlay_queue.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/overlays/overlay_coordinator+internal.h"
#include "ios/chrome/browser/ui/overlays/overlay_coordinator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

OverlayQueue::OverlayQueue()
    : overlays_([[NSMutableArray<OverlayCoordinator*> alloc] init]),
      showing_overlay_(false) {}

OverlayQueue::~OverlayQueue() {
  CancelOverlays();
}

void OverlayQueue::AddObserver(OverlayQueueObserver* observer) {
  observers_.AddObserver(observer);
}

void OverlayQueue::RemoveObserver(OverlayQueueObserver* observer) {
  observers_.RemoveObserver(observer);
}

void OverlayQueue::OverlayWasStopped(OverlayCoordinator* overlay_coordinator) {
  DCHECK(overlay_coordinator);
  DCHECK_EQ(GetFirstOverlay(), overlay_coordinator);
  DCHECK(IsShowingOverlay());
  [overlays_ removeObjectAtIndex:0];
  showing_overlay_ = false;
  for (auto& observer : observers_) {
    observer.OverlayQueueDidStopVisibleOverlay(this);
  }
}

void OverlayQueue::ReplaceVisibleOverlay(
    OverlayCoordinator* overlay_coordinator) {
  DCHECK(overlay_coordinator);
  DCHECK(IsShowingOverlay());
  DCHECK_GE([overlays_ count], 1U);
  // Add the overlay after the currently displayed overlay and notify observers
  // before stopping the current overlay.
  for (auto& observer : observers_) {
    observer.OverlayQueueWillReplaceVisibleOverlay(this);
  }
  [overlays_ insertObject:overlay_coordinator atIndex:1];
  [overlay_coordinator wasAddedToQueue:this];
  OverlayCoordinator* visible_overlay = GetFirstOverlay();
  [visible_overlay cancelOverlay];
  [visible_overlay stop];
}

bool OverlayQueue::HasQueuedOverlays() const {
  return GetCount() > 0U;
}

bool OverlayQueue::IsShowingOverlay() const {
  return showing_overlay_;
}

void OverlayQueue::CancelOverlays() {
  // Cancel all overlays in the queue.
  for (OverlayCoordinator* overlay_coordinator in overlays_) {
    [overlay_coordinator cancelOverlay];
  }
  for (auto& observer : observers_) {
    observer.OverlayQueueDidCancelOverlays(this);
  }
  // Stop the visible overlay before emptying the queue.
  if (IsShowingOverlay())
    [GetFirstOverlay() stop];
  [overlays_ removeAllObjects];
}

web::WebState* OverlayQueue::GetWebState() const {
  return nullptr;
}

void OverlayQueue::AddOverlay(OverlayCoordinator* overlay_coordinator) {
  // Add the overlay coordinator to the queue, then notify observers.
  DCHECK(overlay_coordinator);
  [overlays_ addObject:overlay_coordinator];
  [overlay_coordinator wasAddedToQueue:this];
  for (auto& observer : observers_) {
    observer.OverlayQueueDidAddOverlay(this);
  }
}

NSUInteger OverlayQueue::GetCount() const {
  return [overlays_ count];
}

OverlayCoordinator* OverlayQueue::GetFirstOverlay() {
  return [overlays_ firstObject];
}

void OverlayQueue::OverlayWasStarted() {
  showing_overlay_ = true;
}
