// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MAIN_MAIN_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_MAIN_MAIN_COORDINATOR_H_

#import "ios/chrome/browser/chrome_root_coordinator.h"

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/main/view_controller_swapping.h"

@interface MainCoordinator : ChromeRootCoordinator

// The view controller this coordinator creates and manages.
// (This is only public while the view controller architecture is being
// refactored).
@property(nonatomic, weak, readonly, nullable)
    UIViewController* mainViewController;

@property(nonatomic, weak, readonly, nullable) id<ViewControllerSwapping>
    viewControllerSwapper;

@end

#endif  // IOS_CHROME_BROWSER_UI_MAIN_MAIN_COORDINATOR_H_
