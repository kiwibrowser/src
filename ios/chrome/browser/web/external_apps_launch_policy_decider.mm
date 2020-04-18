// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/external_apps_launch_policy_decider.h"

#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/web/external_app_launching_state.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

const int kMaxAllowedConsecutiveExternalAppLaunches = 2;

@interface ExternalAppsLaunchPolicyDecider ()
// Maps between external application redirection key and state.
// the key is a space separated combination of the absolute string for the
// original source URL, and the scheme of the external Application URL.
@property(nonatomic, strong)
    NSMutableDictionary<NSString*, ExternalAppLaunchingState*>*
        appLaunchingStates;
// Generates key for |appURL| and |sourceURL| to be used to retrieve state from
// |appLaunchingStates|.
+ (NSString*)stateKeyForAppURL:(const GURL&)appURL
                     sourceURL:(const GURL&)sourceURL;
@end

@implementation ExternalAppsLaunchPolicyDecider

@synthesize appLaunchingStates = _appLaunchingStates;

+ (NSString*)stateKeyForAppURL:(const GURL&)appURL
                     sourceURL:(const GURL&)sourcePageURL {
  return
      [NSString stringWithFormat:@"%s %s", sourcePageURL.GetContent().c_str(),
                                 appURL.scheme().c_str()];
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _appLaunchingStates = [[NSMutableDictionary alloc] init];
  }
  return self;
}

- (void)didRequestLaunchExternalAppURL:(const GURL&)gURL
                     fromSourcePageURL:(const GURL&)sourcePageURL {
  NSString* key = [[self class] stateKeyForAppURL:gURL sourceURL:sourcePageURL];
  if (!_appLaunchingStates[key])
    _appLaunchingStates[key] = [[ExternalAppLaunchingState alloc] init];
  [_appLaunchingStates[key] updateWithLaunchRequest];
}

- (ExternalAppLaunchPolicy)launchPolicyForURL:(const GURL&)gURL
                            fromSourcePageURL:(const GURL&)sourcePageURL {
  NSString* key = [[self class] stateKeyForAppURL:gURL sourceURL:sourcePageURL];
  // Don't block apps that are not registered with the policy decider.
  if (!_appLaunchingStates[key])
    return ExternalAppLaunchPolicyAllow;

  ExternalAppLaunchingState* state = _appLaunchingStates[key];
  if ([state isAppLaunchingBlocked])
    return ExternalAppLaunchPolicyBlock;

  if (state.consecutiveLaunchesCount >
      kMaxAllowedConsecutiveExternalAppLaunches) {
    return ExternalAppLaunchPolicyPrompt;
  }
  return ExternalAppLaunchPolicyAllow;
}

- (void)blockLaunchingAppURL:(const GURL&)gURL
           fromSourcePageURL:(const GURL&)sourcePageURL {
  NSString* key = [[self class] stateKeyForAppURL:gURL sourceURL:sourcePageURL];
  [_appLaunchingStates[key] setAppLaunchingBlocked:YES];
}
@end
