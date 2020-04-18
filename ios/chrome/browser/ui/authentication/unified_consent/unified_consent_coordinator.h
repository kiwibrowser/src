// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_UNIFIED_CONSENT_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_UNIFIED_CONSENT_COORDINATOR_H_

#import <UIKit/UIKit.h>

#include <vector>

@class ChromeIdentity;
@class UnifiedConsentCoordinator;

// Delegate protocol for UnifiedConsentCoordinator.
@protocol UnifiedConsentCoordinatorDelegate<NSObject>

// Called when the user taps on the settings link.
- (void)unifiedConsentCoordinatorDidTapSettingsLink:
    (UnifiedConsentCoordinator*)coordinator;

// Called when the user scrolls down to the bottom (or when the view controller
// is loaded with no scroll needed).
- (void)unifiedConsentCoordinatorDidReachBottom:
    (UnifiedConsentCoordinator*)coordinator;

@end

// UnityConsentCoordinator coordinates UnityConsentViewController, which is a
// sub view controller to ask for the user consent before the user can sign-in.
// All the string ids displayed by the view are available with
// |consentStringIds| and |openSettingsStringId|. Those can be used to record
// the consent agreed by the user.
@interface UnifiedConsentCoordinator : NSObject

@property(nonatomic, weak) id<UnifiedConsentCoordinatorDelegate> delegate;
// Identity selected by the user to sign-in. By default, the first identity from
// GetAllIdentitiesSortedForDisplay() is used. If there is no identity in the
// list, the identity picker will be hidden. Nil is not accepted if at least one
// identity exists.
@property(nonatomic, strong) ChromeIdentity* selectedIdentity;
// String id for text to open the settings (related to record the user consent).
@property(nonatomic, readonly) int openSettingsStringId;
// View controller used to display the view.
@property(nonatomic, strong, readonly) UIViewController* viewController;
// Returns YES if the consent view is scrolled to the bottom.
@property(nonatomic, readonly) BOOL isScrolledToBottom;
// Returns YES if the user tapped on the setting link.
@property(nonatomic, readonly) BOOL settingsLinkWasTapped;

// Starts this coordinator.
- (void)start;

// List of string ids used for the user consent. The string ids order matches
// the way they appear on the screen.
- (const std::vector<int>&)consentStringIds;

// Scrolls the consent view to the bottom.
- (void)scrollToBottom;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_UNIFIED_CONSENT_COORDINATOR_H_
