// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_EDIT_CONSUMER_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_EDIT_CONSUMER_H_

#import <UIKit/UIKit.h>

@class EditorField;

// PaymentRequestEditConsumer sets the editor form fields.
@protocol PaymentRequestEditConsumer

// Sets the list of field definitions for the editor.
- (void)setEditorFields:(NSArray<EditorField*>*)fields;

// Sets the options to choose from for |field|. |options| is an array of columns
// which themselves are arrays of string rows used for display in UIPickerView.
- (void)setOptions:(NSArray<NSArray<NSString*>*>*)options
    forEditorField:(EditorField*)field;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_EDIT_CONSUMER_H_
