// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_TOP_TOOLBAR_H_
#define IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_TOP_TOOLBAR_H_

#import <UIKit/UIKit.h>

@class TabGridPageControl;

// Toolbar view with two text buttons and a segmented control. The contents have
// a fixed height and are pinned to the bottom of this view, therefore it is
// intended to be used as a top toolbar.
@interface TabGridTopToolbar : UIView
// These components are publicly available to allow the user to set their
// contents, visibility and actions.
@property(nonatomic, weak, readonly) UIButton* leadingButton;
@property(nonatomic, weak, readonly) UIButton* trailingButton;
@property(nonatomic, weak, readonly) TabGridPageControl* pageControl;
@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_TOP_TOOLBAR_H_
