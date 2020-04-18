// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_APP_LAUNCHER_APP_LAUNCHER_UTIL_H_
#define IOS_CHROME_BROWSER_UI_APP_LAUNCHER_APP_LAUNCHER_UTIL_H_

#import <Foundation/Foundation.h>

class GURL;

// Returns a formatted string that removes the url scheme (e.g., "tel://") and
// percent encoding from the absolute string of |url|.
NSString* GetFormattedAbsoluteUrlWithSchemeRemoved(NSURL* url);

// Returns a set of NSStrings that are URL schemes for iTunes Stores.
NSSet<NSString*>* GetItmsSchemes();

// Returns whether |url| has an app store scheme.
bool UrlHasAppStoreScheme(const GURL& url);

// Returns whether |url| has the scheme of a URL that initiates a call.
bool UrlHasPhoneCallScheme(const GURL& url);

// Returns a string to be used as the label for the prompt's action button.
NSString* GetPromptActionString(NSString* scheme);

#endif  // IOS_CHROME_BROWSER_UI_APP_LAUNCHER_APP_LAUNCHER_UTIL_H_
