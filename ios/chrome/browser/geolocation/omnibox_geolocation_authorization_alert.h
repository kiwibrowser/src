// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_GEOLOCATION_OMNIBOX_GEOLOCATION_AUTHORIZATION_ALERT_H_
#define IOS_CHROME_BROWSER_GEOLOCATION_OMNIBOX_GEOLOCATION_AUTHORIZATION_ALERT_H_

#import <Foundation/Foundation.h>

@class OmniboxGeolocationAuthorizationAlert;

// Defines the methods to be implemented by a delegate of an
// OmniboxGeolocationAuthorizationAlert.
@protocol OmniboxGeolocationAuthorizationAlertDelegate

// Notifies the receiver that the user authorized the use of geolocation for
// Omnibox queries.
- (void)authorizationAlertDidAuthorize:
        (OmniboxGeolocationAuthorizationAlert*)authorizationAlert;

// Notifies the receiver that the user canceled the use of geolocation for
// Omnibox queries.
- (void)authorizationAlertDidCancel:
        (OmniboxGeolocationAuthorizationAlert*)authorizationAlert;

@end

// Presents the alert view that prompts the user to authorize using geolocation
// for Omnibox queries.
@interface OmniboxGeolocationAuthorizationAlert : NSObject

// The delegate for this OmniboxGeolocationAuthorizationAlert.
@property(nonatomic, weak) id<OmniboxGeolocationAuthorizationAlertDelegate>
    delegate;

// Designated initializer. Initializes this instance with |delegate|.
- (instancetype)initWithDelegate:
        (id<OmniboxGeolocationAuthorizationAlertDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;

// Shows the authorization alert.
- (void)showAuthorizationAlert;

@end

#endif  // IOS_CHROME_BROWSER_GEOLOCATION_OMNIBOX_GEOLOCATION_AUTHORIZATION_ALERT_H_
