// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_LEGACY_RECENT_TABS_TABLE_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_LEGACY_RECENT_TABS_TABLE_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/ntp/recent_tabs/recent_tabs_table_consumer.h"

namespace ios {
class ChromeBrowserState;
}

@protocol ApplicationCommands;
@protocol LegacyRecentTabsTableViewControllerDelegate;
@protocol RecentTabsHandsetViewControllerCommand;
@protocol UrlLoader;

// Controls the content of a UITableView.
//
// The UITableView can contain the following different sections:
// A/ Closed tabs section.
//   This section lists all the local tabs that were recently closed.
// A*/ Separator section.
//   This section contains only a single cell that acts as a separator.
// B/ Other Devices section.
//   Depending on the user state, the section will either contain a view
//   offering the user to sign in, a view to activate sync, or a view to inform
//   the user that they can turn on sync on other devices.
// C/ Session section.
//   This section shows the sessions from other devices.
//
// Section A is always present, followed by section A*.
// Depending on the user sync state, either section B or section C will be
// presented.
@interface LegacyRecentTabsTableViewController
    : UITableViewController<UIGestureRecognizerDelegate,
                            RecentTabsTableConsumer>

// Designated initializer. The controller opens link with |loader|.
// |browserState|
// and |loader| must not be nil.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                              loader:(id<UrlLoader>)loader
                          dispatcher:(id<ApplicationCommands>)dispatcher;

// RecentTabsTableViewControllerDelegate delegate.
@property(nonatomic, weak) id<LegacyRecentTabsTableViewControllerDelegate>
    delegate;

// RecentTabsHandsetViewControllerCommand delegate.
@property(nonatomic, weak) id<RecentTabsHandsetViewControllerCommand>
    handsetCommandHandler;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_LEGACY_RECENT_TABS_TABLE_VIEW_CONTROLLER_H_
