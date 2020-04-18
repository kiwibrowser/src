// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_CLIPPING_TEXTFIELD_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_CLIPPING_TEXTFIELD_H_

#import <UIKit/UIKit.h>

@protocol ClippingTextFieldDelegate;

// This textfield subclass is designed to be paired with
// ClippingTextFieldContainer. It notifies its delegate about changes of text
// and becoming/resigning first responder.
@interface ClippingTextField : UITextField

@property(nonatomic, weak) id<ClippingTextFieldDelegate>
    clippingTextfieldDelegate;

@end

@protocol ClippingTextFieldDelegate

// Called when setText: or setAttributedText: was called. Not called when the
// textfield is being typed into.
- (void)textFieldTextChanged:(UITextField*)sender;
- (void)textFieldBecameFirstResponder:(UITextField*)sender;
- (void)textFieldResignedFirstResponder:(UITextField*)sender;

@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_CLIPPING_TEXTFIELD_H_
