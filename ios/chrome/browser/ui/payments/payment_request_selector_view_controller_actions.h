// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_SELECTOR_VIEW_CONTROLLER_ACTIONS_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_SELECTOR_VIEW_CONTROLLER_ACTIONS_H_

// Protocol handling the actions sent by the
// PaymentRequestSelectorViewController.
@protocol PaymentRequestSelectorViewControllerActions

// Called when the user presses the back button.
- (void)onBack;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_SELECTOR_VIEW_CONTROLLER_ACTIONS_H_
