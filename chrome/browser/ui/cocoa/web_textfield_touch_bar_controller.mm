// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/web_textfield_touch_bar_controller.h"

#include "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#import "chrome/browser/ui/cocoa/autofill/credit_card_autofill_touch_bar_controller.h"
#import "chrome/browser/ui/cocoa/tab_contents/tab_contents_controller.h"
#import "ui/base/cocoa/touch_bar_util.h"

@implementation WebTextfieldTouchBarController

- (instancetype)initWithTabContentsController:(TabContentsController*)owner {
  if ((self = [self init])) {
    owner_ = owner;
  }

  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)showCreditCardAutofillWithController:
    (autofill::AutofillPopupController*)controller {
  autofillTouchBarController_.reset(
      [[CreditCardAutofillTouchBarController alloc]
          initWithController:controller]);
  [self invalidateTouchBar];
}

- (void)hideCreditCardAutofillTouchBar {
  autofillTouchBarController_.reset();
  [self invalidateTouchBar];
}

- (void)invalidateTouchBar {
  if ([owner_ respondsToSelector:@selector(setTouchBar:)])
    [owner_ performSelector:@selector(setTouchBar:) withObject:nil];
}

- (NSTouchBar*)makeTouchBar {
  if (autofillTouchBarController_)
    return [autofillTouchBarController_ makeTouchBar];

  return nil;
}

@end
