// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_LOGO_VENDOR_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_LOGO_VENDOR_H_

#import "ios/public/provider/chrome/browser/voice/logo_animation_controller.h"

@class UIView;

// Defines a controller whose view contains a doodle or search engine logo.
@protocol LogoVendor<LogoAnimationControllerOwnerOwner, NSObject>

// View that shows a doodle or a search engine logo.
@property(nonatomic, readonly, retain) UIView* view;

// Whether or not the logo should be shown.  Defaults to YES.
@property(nonatomic, assign, getter=isShowingLogo) BOOL showingLogo;

// Checks for a new doodle.  Calling this method frequently will result in a
// query being issued at most once per hour.
- (void)fetchDoodle;

@end

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_LOGO_VENDOR_H_
