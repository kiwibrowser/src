// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_PICKER_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_PICKER_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/material_components/app_bar_presenting.h"

extern NSString* const kPaymentRequestPickerRowAccessibilityID;
extern NSString* const kPaymentRequestPickerViewControllerAccessibilityID;
extern NSString* const kPaymentRequestPickerSearchBarAccessibilityID;

@class PaymentRequestPickerViewController;
@class PickerRow;

// Delegate protocol for PaymentRequestPickerViewController.
@protocol PaymentRequestPickerViewControllerDelegate<NSObject>

// Notifies the delegate that the user has selected a row.
- (void)paymentRequestPickerViewController:
            (PaymentRequestPickerViewController*)controller
                              didSelectRow:(PickerRow*)row;

// Notifies the delegate that the user has chosen to return to the previous
// screen without making a selection.
- (void)paymentRequestPickerViewControllerDidFinish:
    (PaymentRequestPickerViewController*)controller;
@end

// TableViewController that displays a searchable list of rows featuring a
// selected row as well as an index list.
@interface PaymentRequestPickerViewController
    : UITableViewController<AppBarPresenting>

// The delegate to be notified when the user selects a row.
@property(nonatomic, weak) id<PaymentRequestPickerViewControllerDelegate>
    delegate;

// Initializes the tableView with a list of rows and an optional selected row.
- (instancetype)initWithRows:(NSArray<PickerRow*>*)rows
                    selected:(PickerRow*)row NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;
- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;
- (instancetype)initWithStyle:(UITableViewStyle)style NS_UNAVAILABLE;
@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_PICKER_VIEW_CONTROLLER_H_
