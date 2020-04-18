// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_VIEW_CONTROLLER_SELECTION_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_VIEW_CONTROLLER_SELECTION_DELEGATE_H_

#import <UIKit/UIKit.h>

@class IdentityChooserViewController;

// Delegate protocol for mediator of IdentityChooserViewController.
@protocol IdentityChooserViewControllerSelectionDelegate<NSObject>

// Called when a user select a new identity.
- (void)identityChooserViewController:
            (IdentityChooserViewController*)viewController
         didSelectIdentityAtIndexPath:(NSIndexPath*)index;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_VIEW_CONTROLLER_SELECTION_DELEGATE_H_
