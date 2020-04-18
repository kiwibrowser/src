// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTHENTICATION_SIGNIN_ACCOUNT_SELECTOR_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_AUTHENTICATION_SIGNIN_ACCOUNT_SELECTOR_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/collection_view_controller.h"

@class ChromeIdentity;
@class SigninAccountSelectorViewController;

@protocol SigninAccountSelectorViewControllerDelegate

// Informs the delegate that the user has tapped on the add account button.
- (void)accountSelectorControllerDidSelectAddAccount:
    (SigninAccountSelectorViewController*)accountSelectorController;

@end

// A collection view controller that allows the user to select the account to be
// signed in.
@interface SigninAccountSelectorViewController : CollectionViewController

- (instancetype)init NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;

@property(nonatomic, weak) id<SigninAccountSelectorViewControllerDelegate>
    delegate;

// Returns the identity that is selected or nil.
- (ChromeIdentity*)selectedIdentity;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTHENTICATION_SIGNIN_ACCOUNT_SELECTOR_VIEW_CONTROLLER_H_
