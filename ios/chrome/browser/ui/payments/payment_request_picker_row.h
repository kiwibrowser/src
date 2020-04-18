// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_PICKER_ROW_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_PICKER_ROW_H_

#import <Foundation/Foundation.h>

// The model object for a row in the payment request picker.
@interface PickerRow : NSObject

// The label for the row. This is displayed in the UI.
@property(nonatomic, copy) NSString* label;
// The value for the row. This is not displayed in the UI and is optional.
@property(nonatomic, copy) NSString* value;

- (instancetype)initWithLabel:(NSString*)label value:(NSString*)value;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_PICKER_ROW_H_
