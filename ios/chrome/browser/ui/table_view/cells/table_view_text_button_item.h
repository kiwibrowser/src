// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TABLE_VIEW_CELLS_TABLE_VIEW_TEXT_BUTTON_ITEM_H_
#define IOS_CHROME_BROWSER_UI_TABLE_VIEW_CELLS_TABLE_VIEW_TEXT_BUTTON_ITEM_H_

#import "ios/chrome/browser/ui/table_view/cells/table_view_item.h"

@protocol TextButtonItemDelegate<NSObject>
// Delegates an action to be performed by the presenter.
- (void)performButtonAction;

@end

// TableViewTextButtonItem contains the model for
// TableViewTextButtonCell.
@interface TableViewTextButtonItem : TableViewItem

// EnableSyncActionDelegate to perform TableViewTextButtonCell actions.
@property(nonatomic, weak) id<TextButtonItemDelegate> delegate;
// Text being displayed above the button.
@property(nonatomic, readwrite, strong) NSString* text;
// Text for cell button.
@property(nonatomic, readwrite, strong) NSString* buttonText;

@end

// TableViewTextButtonCell contains a textLabel and a UIbutton
// laid out vertically and centered.
@interface TableViewTextButtonCell : UITableViewCell

// Delegate used to show sync settings options.
@property(nonatomic, weak) id<TextButtonItemDelegate> delegate;
// Cell text information.
@property(nonatomic, strong) UILabel* textLabel;
// Action button.
@property(nonatomic, strong) UIButton* button;

@end

#endif  // IOS_CHROME_BROWSER_UI_TABLE_VIEW_CELLS_TABLE_VIEW_TEXT_BUTTON_ITEM_H_
