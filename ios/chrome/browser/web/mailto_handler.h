// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _IOS_CHROME_BROWSER_WEB_MAILTO_HANDLER_H_
#define _IOS_CHROME_BROWSER_WEB_MAILTO_HANDLER_H_

#import <Foundation/Foundation.h>

class GURL;

// MailtoHandler is the base class for Mail client apps that can handle
// mailto: URLs via custom URL schemes. To add support for Mail clients,
// subclass from this and add it to MailtoHandlerManager class.
@interface MailtoHandler : NSObject

// Name of Mail client. This is a name that a user can recognize, e.g. @"Gmail".
@property(nonatomic, readonly) NSString* appName;

// The iOS App Store ID for the Mail client. This is usually a string of digits.
@property(nonatomic, readonly) NSString* appStoreID;

// Initializer that subclasses should call from -init.
- (instancetype)initWithName:(NSString*)appName
                  appStoreID:(NSString*)appStoreID;

// Returns whether the Mail client app is installed.
- (BOOL)isAvailable;

// Returns the prefix to use with -rewriteMailtoURL:. This is usually the custom
// URL scheme plus some operator prefix. Subclasses should override this method.
- (NSString*)beginningScheme;

// Rewrites |gURL| into a URL with a different URL scheme that will cause a
// native iOS app to be launched to handle the mailto: URL. Returns nil if
// |gURL| is not a mailto: URL. Base class implementation provides the typical
// use which rewrites mailto:user@domain.com?subject=arg to
// mailtoScheme:/co?to=user@domain.com&subject=arg
- (NSString*)rewriteMailtoURL:(const GURL&)gURL;

@end

#endif  // _IOS_CHROME_BROWSER_WEB_MAILTO_HANDLER_H_
