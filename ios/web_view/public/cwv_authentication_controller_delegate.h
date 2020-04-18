// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_AUTHENTICATION_CONTROLLER_DELEGATE_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_AUTHENTICATION_CONTROLLER_DELEGATE_H_

#import <Foundation/Foundation.h>

#import "cwv_export.h"

NS_ASSUME_NONNULL_BEGIN

// |accessToken| OAuth2 access token.
// |expirationDate| Expiration date of |accessToken|.
// |error| Error object to provide if |accessToken| was unable to fetched.
typedef void (^TokenCompletionHandler)(NSString* _Nullable accessToken,
                                       NSDate* _Nullable expirationDate,
                                       NSError* _Nullable error);

@class CWVAuthenticationController;

CWV_EXPORT
@protocol CWVAuthenticationControllerDelegate<NSObject>

// Called when ChromeWebView needs an access token.
// See go/ios-sso-library for documentation on the following parameters.
// |gaiaID| The GaiaID of the user whose access token is requested.
// |clientID| The clientID of ChromeWebView. Used to verify it is the same as
// the one passed to CWVWebView and SSO.
// |scopes| The OAuth scopes requested.
// |completionHandler| Used to return access tokens, expiration date, and error.
- (void)authenticationController:(CWVAuthenticationController*)controller
         getAccessTokenForGaiaID:(NSString*)gaiaID
                        clientID:(NSString*)clientID
                          scopes:(NSArray<NSString*>*)scopes
               completionHandler:(TokenCompletionHandler)completionHandler;

// Called when ChromeWebView needs a list of all SSO identities.
// Every identity returned here may be reflected in google web properties.
// This will not be called unless signed in.
// Must at least contain signed in user.
- (NSArray<CWVIdentity*>*)allIdentitiesForAuthenticationController:
    (CWVAuthenticationController*)controller;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTHENTICATION_CONTROLLER_DELEGATE_H_
