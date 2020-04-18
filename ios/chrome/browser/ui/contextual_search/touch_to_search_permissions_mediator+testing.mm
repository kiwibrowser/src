// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/contextual_search/touch_to_search_permissions_mediator+testing.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios {
class ChromeBrowserState;
}

static BOOL isTouchToSearchAvailable = YES;

@implementation MockTouchToSearchPermissionsMediator
@synthesize canSendPageURLs;
@synthesize canPreloadSearchResults;
@synthesize areContextualSearchQueriesSupported;
@synthesize isVoiceOverEnabled;
@synthesize canExtractTapContextForAllURLs;

// Synthesize preference state getter/setter to bypass superclass
// implementation.
@synthesize preferenceState;

+ (void)setIsTouchToSearchAvailableOnDevice:(BOOL)available {
  isTouchToSearchAvailable = available;
}

+ (BOOL)isTouchToSearchAvailableOnDevice {
  return isTouchToSearchAvailable;
}

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState {
  if ((self = [super initWithBrowserState:browserState])) {
    // Default values mimic everything being allowed.
    self.preferenceState = TouchToSearch::ENABLED;
    self.canSendPageURLs = YES;
    self.canPreloadSearchResults = YES;
    self.areContextualSearchQueriesSupported = YES;
    self.isVoiceOverEnabled = NO;
    self.canExtractTapContextForAllURLs = YES;
  }
  return self;
}

- (void)setUpBrowserState:(ios::ChromeBrowserState*)browserState {
  // no-op, overriding superclass method.
}

- (BOOL)canExtractTapContextForURL:(const GURL&)url {
  return canExtractTapContextForAllURLs;
}

@end
