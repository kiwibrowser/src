// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_COORDINATOR_INTERNAL_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_COORDINATOR_INTERNAL_H_

#import "ios/chrome/browser/ui/overlays/overlay_coordinator.h"

class OverlayQueue;

// Internal interface for use by OverlayService implementation.
@interface OverlayCoordinator (OverlayCoordinatorInternal)

// Tells the OverlayCoordinator that it was added to |queue|.  An
// OverlayCoordinator is expected to only be added to one OverlayQueue in its
// lifetime.
- (void)wasAddedToQueue:(OverlayQueue*)queue;

// Starts overlaying this coordinator over |overlayParent|.  The receiver will
// be added as a child of |overlayParent|.
- (void)startOverlayingCoordinator:(BrowserCoordinator*)overlayParent;

@end

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_COORDINATOR_INTERNAL_H_
