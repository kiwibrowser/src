// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_ACTIVITY_SERVICE_LEGACY_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_ACTIVITY_SERVICE_LEGACY_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"

@protocol ActivityServicePositioner;
@protocol ActivityServicePresentation;
@class CommandDispatcher;
@class TabModel;

namespace ios {
class ChromeBrowserState;
}  // namespace

// ActivityServiceLegacyCoordinator provides a public interface for the share
// menu feature.
@interface ActivityServiceLegacyCoordinator : ChromeCoordinator

// Models.
@property(nonatomic, readwrite, assign) ios::ChromeBrowserState* browserState;
@property(nonatomic, readwrite, weak) CommandDispatcher* dispatcher;
@property(nonatomic, readwrite, weak) TabModel* tabModel;

// Providers.
@property(nonatomic, readwrite, weak) id<ActivityServicePositioner>
    positionProvider;
@property(nonatomic, readwrite, weak) id<ActivityServicePresentation>
    presentationProvider;

// Removes references to any weak objects that this coordinator holds pointers
// to.
- (void)disconnect;

// Cancels any in-progress share activities and dismisses the corresponding UI.
- (void)cancelShare;

@end

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_ACTIVITY_SERVICE_LEGACY_COORDINATOR_H_
