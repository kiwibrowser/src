// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COMMANDS_OPEN_URL_COMMAND_H_
#define IOS_CHROME_BROWSER_UI_COMMANDS_OPEN_URL_COMMAND_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/url_loader.h"

namespace web {
struct Referrer;
}

class GURL;

// A command to open a new tab.
@interface OpenUrlCommand : NSObject

// Mark inherited initializer as unavailable to prevent calling it by mistake.
- (instancetype)init NS_UNAVAILABLE;

// Initializes a command intended to open a URL as a link from a page.
- (instancetype)initWithURL:(const GURL&)url
                   referrer:(const web::Referrer&)referrer
                inIncognito:(BOOL)inIncognito
               inBackground:(BOOL)inBackground
                   appendTo:(OpenPosition)append NS_DESIGNATED_INITIALIZER;

// Initializes a command intended to open a URL from browser chrome (e.g.,
// settings). This will always open in a new foreground tab in non-incognito
// mode.
- (instancetype)initWithURLFromChrome:(const GURL&)url;

// URL to open.
@property(nonatomic, readonly) const GURL& url;

// Referrer for the URL.
@property(nonatomic, readonly) const web::Referrer& referrer;

// Whether this URL command requests opening in incognito or not.
@property(nonatomic, readonly) BOOL inIncognito;

// Whether this URL command requests opening in background or not.
@property(nonatomic, readonly) BOOL inBackground;

// Whether or not this URL command comes from a chrome context (e.g., settings),
// as opposed to a web page context.
@property(nonatomic, readonly) BOOL fromChrome;

// Location where the new tab should be opened.
@property(nonatomic, readonly) OpenPosition appendTo;

@end

#endif  // IOS_CHROME_BROWSER_UI_COMMANDS_OPEN_URL_COMMAND_H_
