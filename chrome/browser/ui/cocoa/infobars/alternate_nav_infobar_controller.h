// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_INFOBARS_ALTERNATE_NAV_INFOBAR_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_INFOBARS_ALTERNATE_NAV_INFOBAR_CONTROLLER_H_

#import "chrome/browser/ui/cocoa/infobars/infobar_controller.h"

@interface AlternateNavInfoBarController : InfoBarController
// Called when there is a click on the link in the infobar.
- (void)linkClicked;
@end

#endif  // CHROME_BROWSER_UI_COCOA_INFOBARS_ALTERNATE_NAV_INFOBAR_CONTROLLER_H_
