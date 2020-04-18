// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"

@class ChromeIdentity;
@class IdentityChooserCoordinator;

// Delegate protocol for IdentityChooserCoordinator.
@protocol IdentityChooserCoordinatorDelegate<NSObject>

// Called when the view controller is closed.
- (void)identityChooserCoordinatorDidClose:
    (IdentityChooserCoordinator*)coordinator;

@end

// Coordinator to display the identity chooser view controller.
@interface IdentityChooserCoordinator : ChromeCoordinator

// Selected ChromeIdentity.
@property(nonatomic, strong) ChromeIdentity* selectedIdentity;
// Delegate.
@property(nonatomic, weak) id<IdentityChooserCoordinatorDelegate> delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_COORDINATOR_H_
