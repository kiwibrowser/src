// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TABLE_VIEW_CELLS_TABLE_VIEW_TEXT_ITEM_H_
#define IOS_CHROME_BROWSER_UI_TABLE_VIEW_CELLS_TABLE_VIEW_TEXT_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/table_view/cells/table_view_item.h"

// Defines the colors used for the cell text.
typedef NS_ENUM(UInt32, TextItemColor) {
  TextItemColorLightGrey = 0x6D6D72,
  TextItemColorBlack = 0x000000,
};

// TableViewTextItem contains the model data for a TableViewTextCell.
@interface TableViewTextItem : TableViewItem

// Text Alignment for the cell's textLabel. Default is NSTextAlignmentLeft.
@property(nonatomic, assign) NSTextAlignment textAlignment;

// Hex color for the cell's textLabel. Default is TextItemColorLightGrey.
@property(nonatomic, assign) TextItemColor textColor;

@property(nonatomic, readwrite, strong) NSString* text;

@end

// UITableViewCell that displays a text label.
@interface TableViewTextCell : UITableViewCell

// The text to display.
@property(nonatomic, readonly, strong) UILabel* textLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_TABLE_VIEW_CELLS_TABLE_VIEW_TEXT_ITEM_H_
