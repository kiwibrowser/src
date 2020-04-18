// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_BOTTOM_TOOLBAR_H_
#define IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_BOTTOM_TOOLBAR_H_

#import <UIKit/UIKit.h>

@class TabGridNewTabButton;

// Toolbar view with three buttons. The contents have a fixed height and are
// pinned to the top of this view, therefore it is intended to be used as a
// bottom toolbar.
@interface TabGridBottomToolbar : UIView
// These components are publicly available to allow the user to set their
// contents, visibility and actions.
@property(nonatomic, weak, readonly) UIButton* leadingButton;
@property(nonatomic, weak, readonly) UIButton* trailingButton;
@property(nonatomic, weak, readonly) TabGridNewTabButton* centerButton;
@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_BOTTOM_TOOLBAR_H_
