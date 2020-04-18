// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MAIN_BROWSER_VIEW_INFORMATION_H_
#define IOS_CHROME_BROWSER_UI_MAIN_BROWSER_VIEW_INFORMATION_H_

#import <Foundation/Foundation.h>

@class BrowserViewController;
@class TabModel;

namespace ios {
class ChromeBrowserState;
}

// Information about the Browser View, controllers and tab model.
@protocol BrowserViewInformation<NSObject>

// The normal (non-OTR) BrowserViewController
@property(nonatomic, retain) BrowserViewController* mainBVC;
// The normal (non-OTR) TabModel corresponding to mainBVC.
@property(nonatomic, retain) TabModel* mainTabModel;
// The OTR BrowserViewController.
@property(nonatomic, retain) BrowserViewController* otrBVC;
// The OTR TabModel corresponding to otrBVC.
@property(nonatomic, retain) TabModel* otrTabModel;
// The BrowserViewController that is currently being used (one of mainBVC or
// otrBVC). The other, if present, is in suspended mode.
@property(nonatomic, assign) BrowserViewController* currentBVC;

// Returns the browser state corresponding to the current browser view.
- (ios::ChromeBrowserState*)currentBrowserState;

// Returns the tab model corresponding to the current browser view.
- (TabModel*)currentTabModel;

// Clean up the device sharing manager.
- (void)cleanDeviceSharingManager;

@end

#endif  // IOS_CHROME_BROWSER_UI_MAIN_BROWSER_VIEW_INFORMATION_H_
