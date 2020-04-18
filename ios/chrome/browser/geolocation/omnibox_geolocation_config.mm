// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/geolocation/omnibox_geolocation_config.h"

#include <set>
#include <string>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

NSString* const kEligibleDomainsKey = @"EligibleDomains";

}  // namespace

@interface OmniboxGeolocationConfig () {
  // Whitelist of domains eligible for Omnibox geolocation.
  std::set<std::string> _eligibleDomains;
}

// Loads |_eligibleDomains| from config file |plistFileName|.
- (void)loadConfigFile:(NSString*)plistFileName;

@end

@implementation OmniboxGeolocationConfig

+ (OmniboxGeolocationConfig*)sharedInstance {
  static OmniboxGeolocationConfig* instance =
      [[OmniboxGeolocationConfig alloc] init];
  return instance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    [self loadConfigFile:@"OmniboxGeolocation"];
  }
  return self;
}

- (BOOL)URLHasEligibleDomain:(const GURL&)url {
  // Here we iterate through the eligible domains and check url.DomainIs() for
  // each domain rather than extracting url.host() and checking the domain for
  // membership in |eligibleDomains_|, because GURL::DomainIs() is more robust
  // and contains logic that we want to reuse.
  for (const auto& eligibleDomain : _eligibleDomains) {
    if (url.DomainIs(eligibleDomain.c_str()))
      return YES;
  }
  return NO;
}

#pragma mark - Private

- (void)loadConfigFile:(NSString*)plistFileName {
  NSString* path = [[NSBundle mainBundle] pathForResource:plistFileName
                                                   ofType:@"plist"
                                              inDirectory:@"gm-config/ANY"];
  NSDictionary* configData = [NSDictionary dictionaryWithContentsOfFile:path];
  if (!configData) {
    // The plist is not packaged with Chromium builds.  This is not an error, so
    // simply return early, since no domains are eligible for geolocation.
    return;
  }

  NSArray* eligibleDomains = base::mac::ObjCCastStrict<NSArray>(
      [configData objectForKey:kEligibleDomainsKey]);
  if (eligibleDomains) {
    for (id item in eligibleDomains) {
      NSString* domain = base::mac::ObjCCastStrict<NSString>(item);
      if ([domain length]) {
        _eligibleDomains.insert(
            base::SysNSStringToUTF8([domain lowercaseString]));
      }
    }
  }
  // Make sure that if a plist exists, it contains at least one eligible domain.
  DCHECK(!_eligibleDomains.empty());
}

@end
