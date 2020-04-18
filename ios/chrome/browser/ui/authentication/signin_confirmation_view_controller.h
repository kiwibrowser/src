// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTHENTICATION_SIGNIN_CONFIRMATION_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_AUTHENTICATION_SIGNIN_CONFIRMATION_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#include <vector>

#import "ios/chrome/browser/ui/collection_view/collection_view_controller.h"

extern NSString* const kSigninConfirmationCollectionViewId;

@class ChromeIdentity;
@class SigninConfirmationViewController;

@protocol SigninConfirmationViewControllerDelegate

// Informs the delegate that the link to open the sync settings was tapped.
- (void)signinConfirmationControllerDidTapSettingsLink:
    (SigninConfirmationViewController*)controller;

// Informs the delegate that the confirmation view has been scrolled all
// the way to the bottom.
- (void)signinConfirmationControllerDidReachBottom:
    (SigninConfirmationViewController*)controller;

@end

// Controller of the sign-in confirmation collection view.
@interface SigninConfirmationViewController : CollectionViewController

@property(nonatomic, weak) id<SigninConfirmationViewControllerDelegate>
    delegate;

// String id for text to open the settings.
@property(nonatomic, readonly) int openSettingsStringId;

- (instancetype)initWithIdentity:(ChromeIdentity*)identity
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;

// Scrolls the confirmation view to the bottom of its content.
- (void)scrollToBottom;

// List of string ids used for the user consent. The string ids order matches
// the way they appear on the screen.
- (const std::vector<int>&)consentStringIds;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTHENTICATION_SIGNIN_CONFIRMATION_VIEW_CONTROLLER_H_
