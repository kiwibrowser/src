// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_GEOLOCATION_OMNIBOX_GEOLOCATION_CONFIG_H_
#define IOS_CHROME_BROWSER_GEOLOCATION_OMNIBOX_GEOLOCATION_CONFIG_H_

#import <Foundation/Foundation.h>

class GURL;

// Implements configuration for using geolocation for Omnibox queries.
@interface OmniboxGeolocationConfig : NSObject

// Returns singleton object for this class.
+ (OmniboxGeolocationConfig*)sharedInstance;

// Returns YES if and only if |url| has a domain that is whitelisted for
// geolocation for Omnibox queries.
- (BOOL)URLHasEligibleDomain:(const GURL&)url;

@end

#endif  // IOS_CHROME_BROWSER_GEOLOCATION_OMNIBOX_GEOLOCATION_CONFIG_H_
