// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MAIN_MAIN_CONTAINING_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_MAIN_MAIN_CONTAINING_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/main/view_controller_swapping.h"

// A UIViewController that uses containment to display TabSwitchers and
// BrowserViewControllers..
@interface MainContainingViewController
    : UIViewController<ViewControllerSwapping>
@end

#endif  // IOS_CHROME_BROWSER_UI_MAIN_MAIN_CONTAINING_VIEW_CONTROLLER_H_
