// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COORDINATORS_TEST_BROWSER_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_COORDINATORS_TEST_BROWSER_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"

// A test version of BrowserCoordinator.  This class backs its |viewController|
// property using a default UIViewController.  It can be used as a parent
// coordinator with no functionality in tests for coordinator lifecycle events.
@interface TestBrowserCoordinator : BrowserCoordinator
@end

#endif  // IOS_CHROME_BROWSER_UI_COORDINATORS_TEST_BROWSER_COORDINATOR_H_
