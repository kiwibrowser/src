// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_TABLE_VIEW_CONTROLLER_COMMANDS_H_
#define IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_TABLE_VIEW_CONTROLLER_COMMANDS_H_

// Delegate for the table view.
@protocol PopupMenuTableViewControllerCommands
// Adds the current page to the reading list.
- (void)readPageLater;
// Navigates to the page associated with |item|.
- (void)navigateToPageForItem:(TableViewItem<PopupMenuItem>*)item;
@end

#endif  // IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_TABLE_VIEW_CONTROLLER_COMMANDS_H_
