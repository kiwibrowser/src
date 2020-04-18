// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"

// A BrowserCoordinator that can be presented with OverlayService.  This class
// is meant to be subclassed to display ephemeral UI with OverlayService that
// can easily be torn down in favor of other browser functions.
@interface OverlayCoordinator : BrowserCoordinator

// Performs cleanup tasks for an OverlayCoordinator whose presentation is
// cancelled via the OverlayService API.  This allows for deterministic cleanup
// to occur for coordinators whose UI has not been started. Rather than relying
// on |-dealloc| to perform cleanup, |-cancelOverlay| can be used to perform
// cleanup tasks deterministically.  The base class implementation does nothing.
- (void)cancelOverlay;

@end

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_COORDINATOR_H_
