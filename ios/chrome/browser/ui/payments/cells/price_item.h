// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PRICE_ITEM_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PRICE_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/chrome/browser/ui/payments/cells/payments_is_selectable.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// PriceItem is the model class corresponding to PriceCell.
@interface PriceItem : CollectionViewItem<PaymentsIsSelectable>

// The leading item string.
@property(nonatomic, copy) NSString* item;

// The middle notification string.
@property(nonatomic, copy) NSString* notification;

// The trailing price string.
@property(nonatomic, copy) NSString* price;

@end

// PriceCell implements an MDCCollectionViewCell subclass containing three text
// labels: an "item" label, a "notification" label, and a "price" label. Labels
// are laid out side-by-side and fill the full width of the cell. If there is
// sufficient room (after accounting for margins) for full widths of all the
// labels, their full widths will be used. Otherwise, unless the price label
// needs more than 50% of the available room, it won't get truncated. Then,
// unless the notification label needs more than 50% of the remaining room, it
// won't get truncated either. The item label then takes up the remaining room.
@interface PriceCell : MDCCollectionViewCell

// UILabels corresponding to |item|, |notification|, and |price| from the item.
@property(nonatomic, readonly, strong) UILabel* itemLabel;
@property(nonatomic, readonly, strong) UILabel* notificationLabel;
@property(nonatomic, readonly, strong) UILabel* priceLabel;

@end

@interface PriceCell (TestingOnly)

// Exposed for testing.
@property(nonatomic, readonly) CGFloat itemLabelTargetWidth;
@property(nonatomic, readonly) CGFloat notificationLabelTargetWidth;
@property(nonatomic, readonly) CGFloat priceLabelTargetWidth;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PRICE_ITEM_H_
