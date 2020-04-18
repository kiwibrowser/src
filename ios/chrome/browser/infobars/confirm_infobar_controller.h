// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INFOBARS_CONFIRM_INFOBAR_CONTROLLER_H_
#define IOS_CHROME_BROWSER_INFOBARS_CONFIRM_INFOBAR_CONTROLLER_H_

#import "ios/chrome/browser/infobars/infobar_controller.h"

class ConfirmInfoBarDelegate;

@interface ConfirmInfoBarController : InfoBarController

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithInfoBarDelegate:(ConfirmInfoBarDelegate*)delegate;

@end

#endif  // IOS_CHROME_BROWSER_INFOBARS_CONFIRM_INFOBAR_CONTROLLER_H_
