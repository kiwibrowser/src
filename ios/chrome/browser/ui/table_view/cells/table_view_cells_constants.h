// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TABLE_VIEW_CELLS_TABLE_VIEW_CELLS_CONSTANTS_H_
#define IOS_CHROME_BROWSER_UI_TABLE_VIEW_CELLS_TABLE_VIEW_CELLS_CONSTANTS_H_

#import <UIKit/UIKit.h>

// The horizontal spacing between views and the container view of a cell.
extern const CGFloat kTableViewHorizontalSpacing;

// The vertical spacing between views and the container view of a cell
extern const CGFloat kTableViewVerticalSpacing;

// The horizontal spacing between subviews within the container view.
extern const CGFloat kTableViewSubViewHorizontalSpacing;

// Animation duration for highlighting selected section header.
extern const CGFloat kTableViewCellSelectionAnimationDuration;

// Color and alpha used to highlight a cell with a middle gray color to
// represent a user tap.
extern const CGFloat kTableViewHighlightedCellColor;
extern const CGFloat kTableViewHighlightedCellColorAlpha;

// Setting the font size to 0 for a custom preferred font lets iOS manage
// sizing.
extern const CGFloat kUseDefaultFontSize;

// Spacing between text label and cell contentView.
extern const CGFloat kTableViewLabelVerticalSpacing;

#endif  // IOS_CHROME_BROWSER_UI_TABLE_VIEW_CELLS_TABLE_VIEW_CELLS_CONSTANTS_H_
