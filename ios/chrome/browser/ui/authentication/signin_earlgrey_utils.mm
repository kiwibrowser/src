// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/signin_earlgrey_utils.h"

#import <EarlGrey/EarlGrey.h>

#include "base/strings/sys_string_conversions.h"
#include "components/signin/core/browser/signin_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/public/provider/chrome/browser/signin/chrome_identity.h"
#import "ios/public/provider/chrome/browser/signin/fake_chrome_identity.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using chrome_test_util::PrimarySignInButton;
using chrome_test_util::SecondarySignInButton;

@implementation SigninEarlGreyUtils

+ (void)checkSigninPromoVisibleWithMode:(SigninPromoViewMode)mode {
  [self checkSigninPromoVisibleWithMode:mode closeButton:YES];
}

+ (void)checkSigninPromoVisibleWithMode:(SigninPromoViewMode)mode
                            closeButton:(BOOL)closeButton {
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
  [[EarlGrey
      selectElementWithMatcher:grey_allOf(
                                   grey_accessibilityID(kSigninPromoViewId),
                                   grey_sufficientlyVisible(), nil)]
      assertWithMatcher:grey_notNil()];
  [[EarlGrey
      selectElementWithMatcher:grey_allOf(PrimarySignInButton(),
                                          grey_sufficientlyVisible(), nil)]
      assertWithMatcher:grey_notNil()];
  switch (mode) {
    case SigninPromoViewModeColdState:
      [[EarlGrey
          selectElementWithMatcher:grey_allOf(SecondarySignInButton(),
                                              grey_sufficientlyVisible(), nil)]
          assertWithMatcher:grey_nil()];
      break;
    case SigninPromoViewModeWarmState:
      [[EarlGrey
          selectElementWithMatcher:grey_allOf(SecondarySignInButton(),
                                              grey_sufficientlyVisible(), nil)]
          assertWithMatcher:grey_notNil()];
      break;
  }
  if (closeButton) {
    [[EarlGrey
        selectElementWithMatcher:grey_allOf(grey_accessibilityID(
                                                kSigninPromoCloseButtonId),
                                            grey_sufficientlyVisible(), nil)]
        assertWithMatcher:grey_notNil()];
  }
}

+ (void)checkSigninPromoNotVisible {
  [[EarlGrey
      selectElementWithMatcher:grey_allOf(
                                   grey_accessibilityID(kSigninPromoViewId),
                                   grey_sufficientlyVisible(), nil)]
      assertWithMatcher:grey_nil()];
  [[EarlGrey
      selectElementWithMatcher:grey_allOf(PrimarySignInButton(),
                                          grey_sufficientlyVisible(), nil)]
      assertWithMatcher:grey_nil()];
  [[EarlGrey
      selectElementWithMatcher:grey_allOf(SecondarySignInButton(),
                                          grey_sufficientlyVisible(), nil)]
      assertWithMatcher:grey_nil()];
}

+ (ChromeIdentity*)fakeIdentity1 {
  return [FakeChromeIdentity identityWithEmail:@"foo1@gmail.com"
                                        gaiaID:@"foo1ID"
                                          name:@"Fake Foo 1"];
}

+ (ChromeIdentity*)fakeIdentity2 {
  return [FakeChromeIdentity identityWithEmail:@"foo2@gmail.com"
                                        gaiaID:@"foo2ID"
                                          name:@"Fake Foo 2"];
}

+ (ChromeIdentity*)fakeManagedIdentity {
  return [FakeChromeIdentity identityWithEmail:@"foo@managed.com"
                                        gaiaID:@"fooManagedID"
                                          name:@"Fake Managed"];
}

+ (void)assertSignedInWithIdentity:(ChromeIdentity*)identity {
  GREYAssertNotNil(identity, @"Need to give an identity");
  // Required to avoid any problem since the following test is not dependant to
  // UI, and the previous action has to be totally finished before going through
  // the assert.
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];

  ios::ChromeBrowserState* browser_state =
      chrome_test_util::GetOriginalBrowserState();
  AccountInfo info =
      ios::SigninManagerFactory::GetForBrowserState(browser_state)
          ->GetAuthenticatedAccountInfo();

  GREYAssertEqual(base::SysNSStringToUTF8(identity.gaiaID), info.gaia,
                  @"Unexpected Gaia ID of the signed in user [expected = "
                  @"\"%@\", actual = \"%s\"]",
                  identity.gaiaID, info.gaia.c_str());
}

+ (void)assertSignedOut {
  // Required to avoid any problem since the following test is not dependant to
  // UI, and the previous action has to be totally finished before going through
  // the assert.
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];

  ios::ChromeBrowserState* browser_state =
      chrome_test_util::GetOriginalBrowserState();
  GREYAssertFalse(ios::SigninManagerFactory::GetForBrowserState(browser_state)
                      ->IsAuthenticated(),
                  @"Unexpected signed in user");
}

@end
