// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_MAILTO_HANDLER_MANAGER_H_
#define IOS_CHROME_BROWSER_WEB_MAILTO_HANDLER_MANAGER_H_

#import <Foundation/Foundation.h>

@class MailtoHandler;
@class MailtoHandlerManager;
class GURL;

// Key to store selected mailto:// URL handler in NSUserDefaults. The value for
// this key in NSUserDefaults is the default handler ID.
extern NSString* const kMailtoHandlerManagerUserDefaultsKey;

// Protocol that must be implemented by observers of MailtoHandlerManager
// object.
@protocol MailtoHandlerManagerObserver<NSObject>
// The default mailto: handler has changed.
- (void)handlerDidChangeForMailtoHandlerManager:(MailtoHandlerManager*)manager;
@end

// An object that manages the available Mail client apps. The currently selected
// Mail client to handle mailto: URL is stored in a key in NSUserDefaults. If a
// default has not been set in NSUserDefaults, nil may be returned from some of
// the public APIs of MailtoHandlerManager.
@interface MailtoHandlerManager : NSObject

// The unique ID of the Mail client app that handles mailto: URL scheme.
// This has a value of nil if default has not been set.
@property(nonatomic, copy) NSString* defaultHandlerID;

// Array of all the currently supported Mail client apps that claim to handle
// mailto: URL scheme through their own custom defined URL schemes.
@property(nonatomic, strong) NSArray<MailtoHandler*>* defaultHandlers;

// Observer object that will be called when |defaultHandlerID| is changed.
@property(nonatomic, weak) id<MailtoHandlerManagerObserver> observer;

// Returns the ID as a string for the system-provided Mail client app.
+ (NSString*)systemMailApp;

// Convenience method to return a new instance of this class initialized with
// a standard set of MailtoHandlers.
+ (instancetype)mailtoHandlerManagerWithStandardHandlers;

// Returns the name of the application that handles mailto: URLs. Returns nil
// if a default has not been set.
- (NSString*)defaultHandlerName;

// Returns the mailto:// handler app corresponding to |handlerID|. Returns nil
// if there is no handler corresponding to |handlerID|.
- (MailtoHandler*)defaultHandlerByID:(NSString*)handlerID;

// Rewrites |URL| into a new URL that can be "opened" to launch the Mail
// client app. May return nil if |URL| is not a mailto: URL, a mail client
// app has not been selected, or there are no Mail client app available.
- (NSString*)rewriteMailtoURL:(const GURL&)URL;

@end

#endif  // IOS_CHROME_BROWSER_WEB_MAILTO_HANDLER_MANAGER_H_
