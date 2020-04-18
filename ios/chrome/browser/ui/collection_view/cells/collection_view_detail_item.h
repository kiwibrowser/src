// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COLLECTION_VIEW_CELLS_COLLECTION_VIEW_DETAIL_ITEM_H_
#define IOS_CHROME_BROWSER_UI_COLLECTION_VIEW_CELLS_COLLECTION_VIEW_DETAIL_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// CollectionViewDetailItem is the model class corresponding to
// CollectionViewDetailCell.
@interface CollectionViewDetailItem : CollectionViewItem

// The accessory type to display on the trailing edge of the cell.
@property(nonatomic) MDCCollectionViewCellAccessoryType accessoryType;

// The main text string.
@property(nonatomic, copy) NSString* text;

// The detail text string.
@property(nonatomic, copy) NSString* detailText;

@end

// CollectionViewDetailCell implements an MDCCollectionViewCell subclass
// containing two text labels: a "main" label and a "detail" label.  The two
// labels are laid out side-by-side and fill the full width of the cell.  Labels
// are truncated as needed to fit in the cell.
@interface CollectionViewDetailCell : MDCCollectionViewCell

// UILabels corresponding to |text| and |detailText| from the item.
@property(nonatomic, readonly, strong) UILabel* textLabel;
@property(nonatomic, readonly, strong) UILabel* detailTextLabel;

// The amount of horizontal space to provide to each of the labels. These values
// are determined with the following logic:
//
// - If there is sufficient room (after accounting for margins) for the full
//   width of each label, use the current width of each label.
// - If not, use the current width of the main label and a clipped width for the
//   detail label.
// - Unless the main label wants more than 75% of the available width and the
//   detail label wants 25% or less of the available width, in which case use a
//   clipped width for the main label and the current width of the detail label.
// - If both labels want more width than their guaranteed minimums (75% and
//   25%), use the guaranteed minimum amount for each.
//
// Exposed for testing.
@property(nonatomic, readonly) CGFloat textLabelTargetWidth;
@property(nonatomic, readonly) CGFloat detailTextLabelTargetWidth;

@end

#endif  // IOS_CHROME_BROWSER_UI_COLLECTION_VIEW_CELLS_COLLECTION_VIEW_DETAIL_ITEM_H_
