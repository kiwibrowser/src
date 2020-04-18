// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SIGNIN_INTERACTION_SIGNIN_INTERACTION_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SIGNIN_INTERACTION_SIGNIN_INTERACTION_CONTROLLER_H_

#import <Foundation/Foundation.h>

#import "base/ios/block_types.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "ios/chrome/browser/signin/constants.h"

@protocol ApplicationCommands;
namespace ios {
class ChromeBrowserState;
}
@class ChromeIdentity;
@protocol SigninInteractionPresenting;

// Interaction controller for sign-in related operations. This class is mainly a
// proxy for |ChromeSigninViewController|, calling directly
// |ChromeIdentityInteractionManager| for the no-accounts case.
@interface SigninInteractionController : NSObject

// Designated initializer.
// * |browserState| is the current browser state. Must not be nil.
// * |presentationProvider| presents the UI. Must not be nil.
// * |accessPoint| represents the access point that initiated the sign-in.
// * |promoAction| is the action taken on a Signin Promo.
// * |dispatcher| is the dispatcher to be used by this class.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                presentationProvider:(id<SigninInteractionPresenting>)presenter
                         accessPoint:(signin_metrics::AccessPoint)accessPoint
                         promoAction:(signin_metrics::PromoAction)promoAction
                          dispatcher:(id<ApplicationCommands>)dispatcher;

// Starts user sign-in.
// * |identity|, if not nil, the user will be signed in without requiring user
//   input, using this Chrome identity.
// * |completion| will be called when the operation is done, and
//   |succeeded| will notify the caller on whether the user is now signed in.
- (void)signInWithIdentity:(ChromeIdentity*)identity
                completion:(signin_ui::CompletionCallback)completion;

// Re-authenticate the user. This method will always show a sign-in web flow.
// The completion block will be called when the operation is done, and
// |succeeded| will notify the caller on whether the user has been
// correctly re-authenticated.
- (void)reAuthenticateWithCompletion:(signin_ui::CompletionCallback)completion;

// Starts the flow to add an identity via ChromeIdentityInteractionManager.
- (void)addAccountWithCompletion:(signin_ui::CompletionCallback)completion;

// Cancels any current process. Calls the completion callback when done.
- (void)cancel;

// Cancels any current process and dismisses any UI. Calls the completion
// callback when done.
- (void)cancelAndDismiss;

@end

#endif  // IOS_CHROME_BROWSER_UI_SIGNIN_INTERACTION_SIGNIN_INTERACTION_CONTROLLER_H_
