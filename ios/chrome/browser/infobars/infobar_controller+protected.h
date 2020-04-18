// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTROLLER_PROTECTED_H_
#define IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTROLLER_PROTECTED_H_

#import "ios/chrome/browser/infobars/infobar_controller.h"

@interface InfoBarController ()

// Creates and returns a view and lays out all the infobar elements in it. Will
// not add it as a subview yet. This method must be overriden in subclasses.
- (UIView<InfoBarViewSizing>*)viewForFrame:(CGRect)bounds;

@end

#endif  // IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTROLLER_PROTECTED_H_
