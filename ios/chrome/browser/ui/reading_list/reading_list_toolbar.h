// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_TOOLBAR_H_
#define IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_TOOLBAR_H_

#import <UIKit/UIKit.h>

@class ActionSheetCoordinator;
@class ReadingListToolbar;

typedef NS_ENUM(NSInteger, ReadingListToolbarState) {
  NoneSelected,
  OnlyReadSelected,
  OnlyUnreadSelected,
  MixedItemsSelected
};

typedef NS_ENUM(NSInteger, ReadingListToolbarHeight) {
  NormalHeight,
  ExpandedHeight
};

@protocol ReadingListToolbarActions<NSObject>

// Callback for the toolbar mark button.
- (void)markPressed;
// Callback for the toolbar delete button.
- (void)deletePressed;
// Enters editing mode. Updates the toolbar.
- (void)enterEditingModePressed;
// Exits editing mode. Updates the toolbar.
- (void)exitEditingModePressed;

@end

// View at the bottom of the reading list panel that presents options to edit
// the entries. When editing, the interface changes, allowing the user to delete
// them and mark them read/unread.
@interface ReadingListToolbar : UIView

// The toolbar state. The text of the buttons change to reflect the state.
@property(nonatomic, assign) ReadingListToolbarState state;

// Informs the toolbar whether there are read items. The "Delete All Read"
// button will be enabled accordingly.
- (void)setHasReadItem:(BOOL)hasRead;
// Sets the editing mode for the toolbar, showing/hiding buttons accordingly.
- (void)setEditing:(BOOL)editing;
// Returns an empty ActionSheetCoordiantor anchored to the mark button with no
// message and no title.
- (ActionSheetCoordinator*)actionSheetForMarkWithBaseViewController:
    (UIViewController*)viewController;
// Updates the height of the toolbar.
- (void)updateHeight;

@end

#endif  // IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_TOOLBAR_H_
