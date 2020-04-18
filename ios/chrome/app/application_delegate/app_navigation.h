// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_APP_APPLICATION_DELEGATE_APP_NAVIGATION_H_
#define IOS_CHROME_APP_APPLICATION_DELEGATE_APP_NAVIGATION_H_

#import <Foundation/Foundation.h>

#include "base/ios/block_types.h"

namespace ios {
class ChromeBrowserState;
}  // namespace ios

@class SettingsNavigationController;

// Handles the navigation through the application.
@protocol AppNavigation<NSObject>

// Navigation View controller for the settings.
@property(nonatomic, retain)
    SettingsNavigationController* settingsNavigationController;

// Presents a SignedInAccountsViewController for |browserState| on the top view
// controller.
- (void)presentSignedInAccountsViewControllerForBrowserState:
    (ios::ChromeBrowserState*)browserState;

// Closes the settings UI with or without animation, with an optional
// completion block.
- (void)closeSettingsAnimated:(BOOL)animated
                   completion:(ProceduralBlock)completion;

@end

#endif  // IOS_CHROME_APP_APPLICATION_DELEGATE_APP_NAVIGATION_H_
