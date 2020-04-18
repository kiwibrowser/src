// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_VIEW_CONTROLLER_PRESENTATION_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_VIEW_CONTROLLER_PRESENTATION_DELEGATE_H_

#import <UIKit/UIKit.h>

@class IdentityChooserViewController;

// Delegate protocol for presentation events of IdentityChooserViewController.
@protocol IdentityChooserViewControllerPresentationDelegate<NSObject>

// Called when IdentityChooserViewController disappear.
- (void)identityChooserViewControllerDidDisappear:
    (IdentityChooserViewController*)viewController;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_VIEW_CONTROLLER_COORDINATOR_DELEGATE_H_
