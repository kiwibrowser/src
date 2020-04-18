// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTOFILL_SAVE_CARD_INFOBAR_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_AUTOFILL_SAVE_CARD_INFOBAR_CONTROLLER_H_

#import "ios/chrome/browser/infobars/infobar_controller.h"

namespace autofill {
class AutofillSaveCardInfoBarDelegateMobile;
}  // namespace autofill

// Acts as a UIViewController for SaveCardInfoBarView. Creates and adds subviews
// to SaveCardInfoBarView and handles user actions.
@interface SaveCardInfoBarController : InfoBarController

- (instancetype)initWithInfoBarDelegate:
    (autofill::AutofillSaveCardInfoBarDelegateMobile*)delegate
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTOFILL_SAVE_CARD_INFOBAR_CONTROLLER_H_
