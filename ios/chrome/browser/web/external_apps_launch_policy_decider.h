// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_EXTERNAL_APPS_LAUNCH_POLICY_DECIDER_H_
#define IOS_CHROME_BROWSER_WEB_EXTERNAL_APPS_LAUNCH_POLICY_DECIDER_H_

#import <Foundation/Foundation.h>

class GURL;

typedef NS_ENUM(NSInteger, ExternalAppLaunchPolicy) {
  // Allow the application to launch.
  ExternalAppLaunchPolicyAllow = 0,
  // Prompt the user with a dialog, so they can choose wether to block the
  // application or launch it.
  ExternalAppLaunchPolicyPrompt,
  // Block launching the application for this session.
  ExternalAppLaunchPolicyBlock,
};

// The maximum allowed number of consecutive launches of the same app before
// starting to prompt.
extern const int kMaxAllowedConsecutiveExternalAppLaunches;

// This class uses a maps between external application redirection keys and
// external app launching state, so it can unwanted behaviors of repeated
// application launching.
// Each key is a space separated combination of the absloute string for the
// original source URL, and the scheme of the external Application URL.
@interface ExternalAppsLaunchPolicyDecider : NSObject

// Updates the state for external application |gURL| and |sourcePageURL| with a
// new app launch request.
- (void)didRequestLaunchExternalAppURL:(const GURL&)gURL
                     fromSourcePageURL:(const GURL&)sourcePageURL;

// Returns the launching recommendation based on the current state for given
// application |gURL| and |sourcePageURL|.
- (ExternalAppLaunchPolicy)launchPolicyForURL:(const GURL&)gURL
                            fromSourcePageURL:(const GURL&)sourcePageURL;

// Sets the state of the application |gURL| and |sourcePageURL| as blocked for
// the remaining lifetime of this object.
- (void)blockLaunchingAppURL:(const GURL&)gURL
           fromSourcePageURL:(const GURL&)sourcePageURL;
@end

#endif  // IOS_CHROME_BROWSER_WEB_EXTERNAL_APPS_LAUNCH_POLICY_DECIDER_H_
