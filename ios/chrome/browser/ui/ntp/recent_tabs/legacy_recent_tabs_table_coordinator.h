// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_LEGACY_RECENT_TABS_TABLE_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_LEGACY_RECENT_TABS_TABLE_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_panel_protocol.h"

namespace ios {
class ChromeBrowserState;
}

@protocol ApplicationCommands;
@protocol RecentTabsHandsetViewControllerCommand;
@class LegacyRecentTabsTableViewController;
@protocol UrlLoader;

// LegacyRecentTabsTableCoordinator manages the connection between a
// RecentTabsMediator and LegacyRecentTabsTableViewController.
@interface LegacyRecentTabsTableCoordinator : ChromeCoordinator

// Command handler for the command sent when the device is a handset. Nil
// otherwise.
@property(nonatomic, weak) id<RecentTabsHandsetViewControllerCommand>
    handsetCommandHandler;

// Public initializer.
- (instancetype)initWithLoader:(id<UrlLoader>)loader
                  browserState:(ios::ChromeBrowserState*)browserState
                    dispatcher:(id<ApplicationCommands>)dispatcher;

// Private initializer, exposed for testing.
- (instancetype)initWithController:
                    (LegacyRecentTabsTableViewController*)controller
                      browserState:(ios::ChromeBrowserState*)browserState
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
    NS_UNAVAILABLE;
- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                              browserState:
                                  (ios::ChromeBrowserState*)browserState
    NS_UNAVAILABLE;

// The view controller managed by this coordinator.
- (UIViewController*)viewController;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_LEGACY_RECENT_TABS_TABLE_COORDINATOR_H_
