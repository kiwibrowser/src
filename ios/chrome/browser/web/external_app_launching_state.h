// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHING_STATE_H_
#define IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHING_STATE_H_

#import <Foundation/Foundation.h>

// Default maximum allowed seconds between 2 launches to be considered
// consecutive.
extern const double kDefaultMaxSecondsBetweenConsecutiveExternalAppLaunches;

// ExternalAppLaunchingState is a state for a single external application
// represented by timestamp of the last time the app was launched, and the
// number of consecutive launches. Launches are considered consecutive when the
// time difference between them are less than
// |kDefaultMaxSecondsBetweenConsecutiveExternalAppLaunches|.
// The ExternalAppLaunchingState doesn't know the source URL nor the destination
// URL, the ExternalAppsLaunchPolicyDecider object will have an
// ExternalAppLaunchingState object for  each sourceURL/Application Scheme pair.
@interface ExternalAppLaunchingState : NSObject
// The max allowed seconds between 2 launches to be considered consecutive.
@property(class, nonatomic) double maxSecondsBetweenConsecutiveLaunches;
// Counts the number of current consecutive launches for the app.
@property(nonatomic, readonly) int consecutiveLaunchesCount;
// YES if the user blocked this app from launching for the current session.
@property(nonatomic, getter=isAppLaunchingBlocked) BOOL appLaunchingBlocked;

// Updates the state with one more try to open the application, this method will
// check the last time the application was opened and the number of times it was
// opened consecutively then update the state.
- (void)updateWithLaunchRequest;
@end

#endif  // IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHING_STATE_H_
