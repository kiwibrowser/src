// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TABLE_VIEW_CELLS_TABLE_VIEW_ACCESSORY_ITEM_H_
#define IOS_CHROME_BROWSER_UI_TABLE_VIEW_CELLS_TABLE_VIEW_ACCESSORY_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/table_view/cells/table_view_item.h"

// TableViewAccessoryItem contains the model data for a TableViewAccessoryCell.
@interface TableViewAccessoryItem : TableViewItem

// The image in the cell. If nil, won't be added to the view hierarchy.
@property(nonatomic, readwrite, strong) UIImage* image;
// The title label in the cell.
@property(nonatomic, readwrite, copy) NSString* title;

@end

// TableViewAccessoryCell contains a favicon, a title, and an accessory.
@interface TableViewAccessoryCell : UITableViewCell

// The cell favicon imageView.
@property(nonatomic, readonly, strong) UIImageView* imageView;

// The cell title.
@property(nonatomic, readonly, strong) UILabel* titleLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_TABLE_VIEW_CELLS_TABLE_VIEW_ACCESSORY_ITEM_H_
