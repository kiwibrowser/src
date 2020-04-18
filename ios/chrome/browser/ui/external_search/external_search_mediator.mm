// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/external_search/external_search_mediator.h"

#include "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#include "ios/public/provider/chrome/browser/external_search/external_search_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ExternalSearchMediator

@synthesize application = _application;

- (UIApplication*)application {
  return _application ?: UIApplication.sharedApplication;
}

#pragma mark - ExternalSearchCommands

- (void)launchExternalSearch {
  NSURL* externalSearchAppLaunchURL = ios::GetChromeBrowserProvider()
                                          ->GetExternalSearchProvider()
                                          ->GetLaunchURL();
  [self.application openURL:externalSearchAppLaunchURL
                    options:@{}
          completionHandler:nil];
}

@end
