// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_HELPERS_TEST_OVERLAY_PARENT_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_HELPERS_TEST_OVERLAY_PARENT_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"

@class OverlayCoordinator;

// Test BrowserCoordinator subclass that can be used as the parent for overlays.
@interface TestOverlayParentCoordinator : BrowserCoordinator

// The overlay that is currently displayed over this parent.
@property(nonatomic, readonly) OverlayCoordinator* presentedOverlay;

@end

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_HELPERS_TEST_OVERLAY_PARENT_COORDINATOR_H_
