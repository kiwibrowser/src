// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_AUTOFILL_CREDIT_CARD_AUTOFILL_TOUCH_BAR_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_AUTOFILL_CREDIT_CARD_AUTOFILL_TOUCH_BAR_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/touch_bar_forward_declarations.h"

namespace autofill {
class AutofillPopupController;
}

@interface CreditCardAutofillTouchBarController : NSObject<NSTouchBarDelegate> {
  autofill::AutofillPopupController* controller_;  // weak
}

- (instancetype)initWithController:
    (autofill::AutofillPopupController*)controller;

- (NSTouchBar*)makeTouchBar API_AVAILABLE(macos(10.12.2));

- (NSTouchBarItem*)touchBar:(NSTouchBar*)touchBar
      makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
    API_AVAILABLE(macos(10.12.2));

@end

@interface CreditCardAutofillTouchBarController (ExposedForTesting)

- (NSButton*)createCreditCardButtonAtRow:(int)row API_AVAILABLE(macos(10.12.2));
- (void)acceptCreditCard:(id)sender;

@end

#endif  // CHROME_BROWSER_UI_COCOA_AUTOFILL_CREDIT_CARD_AUTOFILL_TOUCH_BAR_CONTROLLER_H_