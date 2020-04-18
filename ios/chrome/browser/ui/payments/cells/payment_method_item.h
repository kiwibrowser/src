// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PAYMENT_METHOD_ITEM_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PAYMENT_METHOD_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/chrome/browser/ui/payments/cells/payments_is_selectable.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// PaymentMethodItem is the model class corresponding to PaymentMethodCell.
@interface PaymentMethodItem : CollectionViewItem<PaymentsIsSelectable>

// A unique identifier for the payment method (for example, the type and last 4
// digits of a credit card).
@property(nonatomic, copy) NSString* methodID;

// Additional details about the payment method (for example, the name of the
// credit card holder).
@property(nonatomic, copy) NSString* methodDetail;

// The address associated with the the payment method (for example, credit
// card's billing address).
@property(nonatomic, copy) NSString* methodAddress;

// The notification message.
@property(nonatomic, copy) NSString* notification;

// An image corresponding to the type of the payment method.
@property(nonatomic, strong) UIImage* methodTypeIcon;

// If YES, reserves room for the accessory type view regardless of whether the
// item has an accessory type. This is used to ensure the content area always
// has the same size regardless of whether the accessory type is set.
@property(nonatomic, assign) BOOL reserveRoomForAccessoryType;

@end

// PaymentMethodCell implements an MDCCollectionViewCell subclass containing
// four optional text labels identifying and providing details about a payment
// method and an image view displaying an icon representing the payment method's
// type. The image is laid out on the trailing edge of the cell while the labels
// are laid out on the leading edge of the cell up to the leading edge of the
// image view. The labels are truncated if necessary.
@interface PaymentMethodCell : MDCCollectionViewCell

// UILabels corresponding to |methodID|, |methodDetail|, |methodAddress|, and
// |notification|.
@property(nonatomic, readonly, strong) UILabel* methodIDLabel;
@property(nonatomic, readonly, strong) UILabel* methodDetailLabel;
@property(nonatomic, readonly, strong) UILabel* methodAddressLabel;
@property(nonatomic, readonly, strong) UILabel* notificationLabel;

@property(nonatomic, assign) BOOL reserveRoomForAccessoryType;

// UIImageView containing the payment method type icon.
@property(nonatomic, readonly, strong) UIImageView* methodTypeIconView;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PAYMENT_METHOD_ITEM_H_
