// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_UTIL_FORM_SHEET_NAVIGATION_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_UTIL_FORM_SHEET_NAVIGATION_CONTROLLER_H_

#import <UIKit/UIKit.h>

// A navigation controller subclass that respects the
// disablesAutomaticKeyboardDismissal of its top view controller. This is useful
// when presenting some view controllers in a navigation controller with
// UIModalPresentationFormSheet.
@interface FormSheetNavigationController : UINavigationController
@end

#endif  // IOS_CHROME_BROWSER_UI_UTIL_FORM_SHEET_NAVIGATION_CONTROLLER_H_
