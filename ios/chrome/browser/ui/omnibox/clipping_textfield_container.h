// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_CLIPPING_TEXTFIELD_CONTAINER_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_CLIPPING_TEXTFIELD_CONTAINER_H_

#import "ios/chrome/browser/ui/omnibox/clipping_textfield.h"

// A container for the textfield that manages clipping it appropriately.
// It will size the textfield to fit precisely when the textfield is first
// responder. Otherwise, it will clip the textfield to its own bounds in the
// best way to fit the most significant part of the URL, like this:
//
//                     --------------------
// www.somereallyreally|longdomainname.com|/path/gets/clipped
//                     --------------------
// {  clipped prefix  } {  visible text  } { clipped suffix }
//
// The clipped parts will also be masked with a gradient fade effect on the
// clipped ends.
@interface ClippingTextFieldContainer : UIView<ClippingTextFieldDelegate>

// The only available initializer. This will immediately add |textField| as its
// subview and set |self| as textField's clipping delegate.
- (instancetype)initWithClippingTextField:(ClippingTextField*)textField
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Managed textfield.
@property(nonatomic, strong, readonly) ClippingTextField* textField;

@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_CLIPPING_TEXTFIELD_CONTAINER_H_
