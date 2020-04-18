// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_GRID_GRID_GRID_CELL_H_
#define IOS_CHROME_BROWSER_UI_TAB_GRID_GRID_GRID_CELL_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/tab_grid/grid/grid_theme.h"

@class GridCell;

// Informs the receiver of actions on the cell.
@protocol GridCellDelegate
- (void)closeButtonTappedForCell:(GridCell*)cell;
@end

// A square-ish cell in a grid. Contains an icon, title, snapshot, and close
// button.
@interface GridCell : UICollectionViewCell
// Delegate to inform the grid of actions on the cell.
@property(nonatomic, weak) id<GridCellDelegate> delegate;
// The look of the cell.
@property(nonatomic, assign) GridTheme theme;
// Unique identifier for the cell's contents. This is used to ensure that
// updates in an asynchronous callback are only made if the item is the same.
@property(nonatomic, copy) NSString* itemIdentifier;
// Settable UI elements of the cell.
@property(nonatomic, weak) UIImage* icon;
@property(nonatomic, weak) UIImage* snapshot;
@property(nonatomic, copy) NSString* title;

// Returns a cell with the same theme, icon, snapshot, and title as the reciever
// (but no delegate or identifier) for use in animated transitions.
- (GridCell*)proxyForTransitions;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_GRID_GRID_GRID_CELL_H_
