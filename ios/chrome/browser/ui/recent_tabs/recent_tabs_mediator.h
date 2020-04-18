// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_RECENT_TABS_RECENT_TABS_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_RECENT_TABS_RECENT_TABS_MEDIATOR_H_

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/sync/synced_sessions_bridge.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/closed_tabs_observer_bridge.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/legacy_recent_tabs_table_view_controller_delegate.h"
#import "ios/chrome/browser/ui/recent_tabs/recent_tabs_image_data_source.h"

namespace ios {
class ChromeBrowserState;
}

@protocol RecentTabsTableConsumer;

// RecentTabsMediator controls the RecentTabsTableConsumer,
// based on the user's signed-in and chrome-sync states.
//
// RecentTabsMediator listens for notifications about Chrome Sync
// and ChromeToDevice and changes/updates the RecentTabsTableConsumer
// accordingly.
@interface RecentTabsMediator
    : NSObject<ClosedTabsObserving,
               RecentTabsImageDataSource,
               SyncedSessionsObserver,
               LegacyRecentTabsTableViewControllerDelegate>

// Starts observing the he user's signed-in and chrome-sync states.
- (void)initObservers;

// Disconnects the mediator from all observers.
- (void)disconnect;

// The consumer for this object. This can change during the lifetime of this
// object and may be nil.
@property(nonatomic, strong) id<RecentTabsTableConsumer> consumer;

// The coordinator's BrowserState.
@property(nonatomic, assign) ios::ChromeBrowserState* browserState;
@end

#endif  // IOS_CHROME_BROWSER_UI_RECENT_TABS_RECENT_TABS_MEDIATOR_H_
