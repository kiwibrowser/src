// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_TEST_OVERLAY_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_TEST_OVERLAY_COORDINATOR_H_

#import "ios/chrome/browser/ui/overlays/overlay_coordinator.h"

// Test OverlayCoordinator.
@interface TestOverlayCoordinator : OverlayCoordinator

// Whether |-cancel| has been called for this coordinator.
@property(nonatomic, readonly, getter=isCancelled) BOOL cancelled;

@end

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_TEST_TEST_OVERLAY_COORDINATOR_H_
