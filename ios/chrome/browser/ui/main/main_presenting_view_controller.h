// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MAIN_MAIN_PRESENTING_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_MAIN_MAIN_PRESENTING_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/main/view_controller_swapping.h"

#import <UIKit/UIKit.h>

@protocol TabSwitcher;

// A UIViewController that uses presentation to display TabSwitchers and
// BrowserViewControllers..
@interface MainPresentingViewController
    : UIViewController<ViewControllerSwapping>

@property(nonatomic, readonly, weak) id<TabSwitcher> tabSwitcher;

// If this property is YES, calls to |showTabSwitcher:completion:| and
// |showTabViewController:completion:| will present the given view controllers
// without animation.  This should only be used by unittests.
@property(nonatomic, readwrite, assign) BOOL animationsDisabledForTesting;

@end

#endif  // IOS_CHROME_BROWSER_UI_MAIN_MAIN_PRESENTING_VIEW_CONTROLLER_H_
