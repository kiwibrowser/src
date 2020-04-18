// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_READING_LIST_MENU_VIEW_ITEM_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_READING_LIST_MENU_VIEW_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/tools_menu/tools_menu_view_item.h"

@class NumberBadgeView;

// Specialization of a ToolsMenuViewItem for the reading list.
@interface ReadingListMenuViewItem : ToolsMenuViewItem
@end

// Specialization of a ToolsMenuViewCell that shows a badge showing the number
// of unread reading list items and sets the menu item title and badge color
// based on existence of unread items.
@interface ReadingListMenuViewCell : ToolsMenuViewCell

// Update the badge count according to |count|. Can be animated or not. If
// |count| is greater than 0 and the text badge is visible, then the text badge
// is animated out, because only one badge should be visible, and the number
// badge takes precedence.
- (void)updateBadgeCount:(NSInteger)count animated:(BOOL)animated;

// Displays or hides the text badge based on |showTextBadge|. Does nothing if
// the number badge is currently visible. Can be animated or not.
- (void)updateShowTextBadge:(BOOL)showTextBadge animated:(BOOL)animated;

// Update the seen state according to |hasUnseenItems|. Can be animated or
// not.
- (void)updateSeenState:(BOOL)hasUnseenItems animated:(BOOL)animated;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_READING_LIST_MENU_VIEW_ITEM_H_
