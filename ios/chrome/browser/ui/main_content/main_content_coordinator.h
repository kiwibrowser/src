// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"
#import "ios/chrome/browser/ui/main_content/main_content_ui.h"

// A coordinator that displays the main content view of the browser.  This
// should be subclassed by any coordinator that displays the scrollable content
// related to the current URL.
@interface MainContentCoordinator : BrowserCoordinator

// The view controller used to display the main content area.
@property(nonatomic, readonly) UIViewController<MainContentUI>* viewController;

@end

#endif  // IOS_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_COORDINATOR_H_
