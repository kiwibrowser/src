// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FIRST_RUN_FIRST_RUN_CHROME_SIGNIN_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_FIRST_RUN_FIRST_RUN_CHROME_SIGNIN_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/authentication/chrome_signin_view_controller.h"

extern NSString* const kSignInButtonAccessibilityIdentifier;
extern NSString* const kSignInSkipButtonAccessibilityIdentifier;

@protocol ApplicationCommands;
@class FirstRunConfiguration;
namespace ios {
class ChromeBrowserState;
}
@protocol SyncPresenter;
@class TabModel;

// A ChromeSigninViewController that is used during the run.
@interface FirstRunChromeSigninViewController : ChromeSigninViewController

// Designated initialzer.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                            tabModel:(TabModel*)tabModel
                      firstRunConfig:(FirstRunConfiguration*)firstRunConfig
                      signInIdentity:(ChromeIdentity*)identity
                           presenter:(id<SyncPresenter>)presenter
                          dispatcher:(id<ApplicationCommands>)dispatcher;

@end

#endif  // IOS_CHROME_BROWSER_UI_FIRST_RUN_FIRST_RUN_CHROME_SIGNIN_VIEW_CONTROLLER_H_
