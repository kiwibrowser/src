// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PROVIDERS_UI_CHROMIUM_STYLED_TEXT_FIELD_H_
#define IOS_CHROME_BROWSER_PROVIDERS_UI_CHROMIUM_STYLED_TEXT_FIELD_H_

#import "ios/public/provider/chrome/browser/ui/text_field_styling.h"

#import <UIKit/UIKit.h>

// ChromiumStyledTextField does not style the text field or perform text
// validation, but it provides a barebones implementation of the
// TextFieldStyling protocol for use in Chromium builds.
@interface ChromiumStyledTextField : UITextField<TextFieldStyling>
@end

#endif  // IOS_CHROME_BROWSER_PROVIDERS_UI_CHROMIUM_STYLED_TEXT_FIELD_H_
