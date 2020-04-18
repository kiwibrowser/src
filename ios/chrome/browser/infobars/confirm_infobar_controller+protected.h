// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INFOBARS_CONFIRM_INFOBAR_CONTROLLER_PROTECTED_H_
#define IOS_CHROME_BROWSER_INFOBARS_CONFIRM_INFOBAR_CONTROLLER_PROTECTED_H_

#import "ios/chrome/browser/infobars/confirm_infobar_controller.h"

@class ConfirmInfoBarView;

@interface ConfirmInfoBarController ()

// Accesses the view.
- (ConfirmInfoBarView*)view;

// Action for any of the user defined buttons.
- (void)infoBarButtonDidPress:(id)sender;
// Action for any of the user defined links.
- (void)infobarLinkDidPress:(NSUInteger)tag;
// Updates the label on the provided view.
- (void)updateInfobarLabel:(ConfirmInfoBarView*)view;

@end

#endif  // IOS_CHROME_BROWSER_INFOBARS_CONFIRM_INFOBAR_CONTROLLER_PROTECTED_H_
