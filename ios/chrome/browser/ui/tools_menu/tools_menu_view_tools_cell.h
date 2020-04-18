// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_VIEW_TOOLS_CELL_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_VIEW_TOOLS_CELL_H_

#import <UIKit/UIKit.h>

// This is intended to be the first cell int the Tools Menu table. It contains
// Toolbar buttons used in compact widths. If using a standard width e.g. iPad
// full screen this cell will not be inserted in the Tools Menu table.
@interface ToolsMenuViewToolsCell : UICollectionViewCell
// Button for refreshing the page. Will be displayed when the context indicates
// that the page has loaded.
@property(nonatomic, strong) UIButton* reloadButton;
// Button that displays the share contextual menu.
@property(nonatomic, strong) UIButton* shareButton;
// Button for bookmarking a page.
@property(nonatomic, strong) UIButton* starButton;
// Button for editing a bookmarked page.
@property(nonatomic, strong) UIButton* starredButton;
// Button for stopping a page from loading. This will only be displayed
// if the page is currently being loaded. Otherwise the reloadButton
// will be shown.
@property(nonatomic, strong) UIButton* stopButton;
// Button for dismissing the Tools Menu.
@property(nonatomic, strong) UIButton* toolsButton;

// All buttons in the cell added in display order.
- (NSArray*)allButtons;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_VIEW_TOOLS_CELL_H_
