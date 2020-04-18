// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ALERT_COORDINATOR_ACTION_SHEET_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_ALERT_COORDINATOR_ACTION_SHEET_COORDINATOR_H_

#import "ios/chrome/browser/ui/alert_coordinator/alert_coordinator.h"

// Coordinator for displaying Action Sheets.
@interface ActionSheetCoordinator : AlertCoordinator

// Init with the parameters for anchoring the popover to a UIView.
- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                     title:(NSString*)title
                                   message:(NSString*)message
                                      rect:(CGRect)rect
                                      view:(UIView*)view
    NS_DESIGNATED_INITIALIZER;

// Init with the parameters for anchoring the popover to a UIBarButtonItem.
- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                     title:(NSString*)title
                                   message:(NSString*)message
                             barButtonItem:(UIBarButtonItem*)barButtonItem
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                     title:(NSString*)title
                                   message:(NSString*)message NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_ALERT_COORDINATOR_ACTION_SHEET_COORDINATOR_H_
