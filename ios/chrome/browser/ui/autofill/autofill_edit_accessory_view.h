// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTOFILL_AUTOFILL_EDIT_ACCESSORY_VIEW_H_
#define IOS_CHROME_BROWSER_UI_AUTOFILL_AUTOFILL_EDIT_ACCESSORY_VIEW_H_

#import <UIKit/UIKit.h>

extern NSString* const kAutofillEditAccessoryViewAccessibilityID;

@protocol AutofillEditAccessoryDelegate<NSObject>
- (void)nextPressed;
- (void)previousPressed;
- (void)closePressed;
@end

// This class is the accessory view in the keyboard for the Autofill editors
// (both profile and credit card).
@interface AutofillEditAccessoryView : UIView

- (instancetype)initWithDelegate:(id<AutofillEditAccessoryDelegate>)delegate;

@property(nonatomic, readonly) UIButton* previousButton;
@property(nonatomic, readonly) UIButton* nextButton;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTOFILL_AUTOFILL_EDIT_ACCESSORY_VIEW_H_
