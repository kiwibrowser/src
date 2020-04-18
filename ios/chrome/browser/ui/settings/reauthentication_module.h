// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_REAUTHENTICATION_MODULE_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_REAUTHENTICATION_MODULE_H_

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/ui/settings/reauthentication_protocol.h"

// A help article on how to set up a passcode.
extern const char kPasscodeArticleURL[];

@protocol SuccessfulReauthTimeAccessor<NSObject>

// Method meant to be called by the |ReauthenticationModule| to update
// the time of the last successful re-authentication.
- (void)updateSuccessfulReauthTime;

// Returns the time of the last successful re-authentication.
- (NSDate*)lastSuccessfulReauthTime;

@end

/**
 * This is used by |PasswordsDetailsCollectionViewController| and
 * |PasswordExporter|to re-authenticate the user before displaying the password
 * in plain text, allowing it to be copied, or exporting passwords.
 */
@interface ReauthenticationModule : NSObject<ReauthenticationProtocol>

// The designated initializer. |successfulReauthTimeAccessor| must not be nil.
- (instancetype)initWithSuccessfulReauthTimeAccessor:
    (id<SuccessfulReauthTimeAccessor>)successfulReauthTimeAccessor
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_REAUTHENTICATION_MODULE_H_
