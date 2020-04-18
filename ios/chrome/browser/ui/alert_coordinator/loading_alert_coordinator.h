// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ALERT_COORDINATOR_LOADING_ALERT_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_ALERT_COORDINATOR_LOADING_ALERT_COORDINATOR_H_

#include "base/ios/block_types.h"
#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"

// Coordinator displaying an activity indicator with a title and a cancel
// button.
@interface LoadingAlertCoordinator : ChromeCoordinator

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
    NS_UNAVAILABLE;
- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                              browserState:
                                  (ios::ChromeBrowserState*)browserState
    NS_UNAVAILABLE;

// Initializes the coordinator with the |viewController| which will present the
// dialog, the |title| of the alert and the |cancelHandler| callback if the
// cancel button is tapped.
// |cancelHandler| can be nil.
- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                     title:(NSString*)title
                             cancelHandler:(ProceduralBlock)cancelHandler
    NS_DESIGNATED_INITIALIZER;

@end

#endif  // IOS_CHROME_BROWSER_UI_ALERT_COORDINATOR_LOADING_ALERT_COORDINATOR_H_
