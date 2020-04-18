// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SAFE_MODE_SAFE_MODE_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_SAFE_MODE_SAFE_MODE_COORDINATOR_H_

#import "ios/chrome/browser/chrome_root_coordinator.h"

#import <UIKit/UIKit.h>

@class SafeModeCoordinator;

@protocol SafeModeCoordinatorDelegate<NSObject>
- (void)coordinatorDidExitSafeMode:(nonnull SafeModeCoordinator*)coordinator;
@end

// Coordinator to manage the Safe Mode UI. This should be self-contained.
// While this is a ChromeCoordinator, it doesn't support (and will DCHECK) using
// child coordinators.
@interface SafeModeCoordinator : ChromeRootCoordinator

// Delegate for this coordinator.
@property(nonatomic, nullable, weak) id<SafeModeCoordinatorDelegate> delegate;

// If YES, there's a reason to show this coordinator.
+ (BOOL)shouldStart;

@end

#endif  // IOS_CHROME_BROWSER_UI_SAFE_MODE_SAFE_MODE_COORDINATOR_H_
