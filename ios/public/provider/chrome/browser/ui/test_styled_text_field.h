// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_TEST_STYLED_TEXT_FIELD_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_TEST_STYLED_TEXT_FIELD_H_

#import "ios/public/provider/chrome/browser/ui/text_field_styling.h"

#import <UIKit/UIKit.h>

@interface TestStyledTextField : UITextField<TextFieldStyling>
@end

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_TEST_STYLED_TEXT_FIELD_H_
