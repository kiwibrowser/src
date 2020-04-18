// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_ACCEPTED_PAYMENT_METHODS_ITEM_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_ACCEPTED_PAYMENT_METHODS_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// AcceptedPaymentMethodsItem is the model class corresponding to
// AcceptedPaymentMethodsCell.
@interface AcceptedPaymentMethodsItem : CollectionViewItem

// The message to be displayed alongside the icons for the accepted payment
// method types.
@property(nonatomic, copy) NSString* message;

// An array of icons corresponding to the accepted payment method types.
@property(nonatomic, strong) NSArray<UIImage*>* methodTypeIcons;

@end

// AcceptedPaymentMethodsCell implements an MDCCollectionViewCell subclass
// containing one text label and a list of icons representing the accepted
// payment method types. The text label is laid out on the leading edge of the
// cell up to the trailing edge of the cell. Below that, icon are laid out in a
// row from the leading edge of the cell up to the trailing edge of the cell.
// The text label is wrapped as needed to fit in the cell.
@interface AcceptedPaymentMethodsCell : MDCCollectionViewCell

// UILabel corresponding to |message|.
@property(nonatomic, readonly, strong) UILabel* messageLabel;

// An array of UIImageView objects containing the icons for the accepted payment
// method types.
@property(nonatomic, strong) NSArray<UIImageView*>* methodTypeIconViews;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_ACCEPTED_PAYMENT_METHODS_ITEM_H_
