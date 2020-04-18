// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COORDINATORS_BROWSER_COORDINATOR_TEST_UTIL_H_
#define IOS_CHROME_BROWSER_UI_COORDINATORS_BROWSER_COORDINATOR_TEST_UTIL_H_

@class BrowserCoordinator;

// Waits for |coordinator|'s UI to finish being presented.
void WaitForBrowserCoordinatorActivation(BrowserCoordinator* coordinator);

// Waits for |coordinator|'s UI to finish being dismissed.
void WaitForBrowserCoordinatorDeactivation(BrowserCoordinator* coordinator);

#endif  // IOS_CHROME_BROWSER_UI_COORDINATORS_BROWSER_COORDINATOR_TEST_UTIL_H_
