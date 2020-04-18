// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_NEW_TAB_BUTTON_H_
#define IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_NEW_TAB_BUTTON_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/tab_grid/tab_grid_paging.h"

// The size class determines the intrinsic size of the button.
typedef NS_ENUM(NSUInteger, TabGridNewTabButtonSizeClass) {
  TabGridNewTabButtonSizeClassSmall = 1,
  TabGridNewTabButtonSizeClassLarge,
};

// The "new tab" button is a button that the user taps when they want to create
// a new tab. Every combination of |sizeClass| and |page| results in a
// differently configured button.
@interface TabGridNewTabButton : UIButton
@property(nonatomic, assign) TabGridPage page;
@property(nonatomic, assign) TabGridNewTabButtonSizeClass sizeClass;
+ (instancetype)buttonWithSizeClass:(TabGridNewTabButtonSizeClass)sizeClass;
@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_NEW_TAB_BUTTON_H_
