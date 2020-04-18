// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTHENTICATION_SIGNIN_EARLGREY_UTILS_H_
#define IOS_CHROME_BROWSER_UI_AUTHENTICATION_SIGNIN_EARLGREY_UTILS_H_

#import "ios/chrome/browser/ui/authentication/signin_promo_view.h"

@class ChromeIdentity;

// Methods used for the EarlGrey tests.
@interface SigninEarlGreyUtils : NSObject

// Checks that the sign-in promo view (with a close button) is visible using the
// right mode.
+ (void)checkSigninPromoVisibleWithMode:(SigninPromoViewMode)mode;

// Checks that the sign-in promo view is visible using the right mode. If
// |closeButton| is set to YES, the close button in the sign-in promo has to be
// visible.
+ (void)checkSigninPromoVisibleWithMode:(SigninPromoViewMode)mode
                            closeButton:(BOOL)closeButton;

// Checks that the sign-in promo view is not visible.
+ (void)checkSigninPromoNotVisible;

// Returns a fake identity.
+ (ChromeIdentity*)fakeIdentity1;

// Returns a second fake identity.
+ (ChromeIdentity*)fakeIdentity2;

// Returns a fake managed identity.
+ (ChromeIdentity*)fakeManagedIdentity;

// Asserts that |identity| is actually signed in to the active profile.
+ (void)assertSignedInWithIdentity:(ChromeIdentity*)identity;

// Asserts that no identity is signed in.
+ (void)assertSignedOut;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTHENTICATION_SIGNIN_EARLGREY_UTILS_H_
