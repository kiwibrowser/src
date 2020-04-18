// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_IDENTITY_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_IDENTITY_H_

#import <Foundation/Foundation.h>

#import "cwv_export.h"

NS_ASSUME_NONNULL_BEGIN

CWV_EXPORT
@interface CWVIdentity : NSObject

- (instancetype)initWithEmail:(NSString*)email
                     fullName:(nullable NSString*)fullName
                       gaiaID:(NSString*)gaiaID NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// Identity/account email address. This can be shown to the user, but is not a
// unique identifier (@see gaiaID).
@property(nonatomic, copy, readonly) NSString* email;

// Returns the full name of the identity.
// Could be nil if no full name has been fetched for this account yet.
@property(nonatomic, copy, readonly, nullable) NSString* fullName;

// The unique GAIA user identifier for this identity/account.
// Use this as a unique identifier to remember a particular identity.
// Use SSOIdentity's |userID| property. See go/ios-sso-library for more info.
@property(nonatomic, copy, readonly) NSString* gaiaID;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_IDENTITY_H_
