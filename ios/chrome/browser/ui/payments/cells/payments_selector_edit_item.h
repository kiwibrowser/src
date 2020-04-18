// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PAYMENTS_SELECTOR_EDIT_ITEM_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PAYMENTS_SELECTOR_EDIT_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/autofill/autofill_ui_type.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/chrome/browser/ui/payments/cells/payments_is_selectable.h"

// Item that configures a CollectionViewDetailCell in order to represent a
// selector-backed editor form field.
@interface PaymentsSelectorEditItem : CollectionViewItem<PaymentsIsSelectable>

// The name of the field.
@property(nonatomic, nullable, copy) NSString* name;

// The value of the field. This is displayed in the UI.
@property(nonatomic, nullable, copy) NSString* value;

// The field type this item is describing.
@property(nonatomic, assign) AutofillUIType autofillUIType;

// Whether this field is required. If YES, an "*" is appended to the name of the
// text field to indicate that the field is required. It is also used for
// validation purposes.
@property(nonatomic, getter=isRequired) BOOL required;

// The font of the name text. Default is the medium Roboto font of size 14.
@property(nonatomic, null_resettable, copy) UIFont* nameFont;

// The color of the name text. Default is the 900 tint color of the grey
// palette.
@property(nonatomic, null_resettable, copy) UIColor* nameColor;

// The font of the value text. Default is the regular Roboto font of size 14.
@property(nonatomic, null_resettable, copy) UIFont* valueFont;

// The color of the value text. Default is the 600 tint color of the blue
// palette.
@property(nonatomic, null_resettable, copy) UIColor* valueColor;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PAYMENTS_SELECTOR_EDIT_ITEM_H_
