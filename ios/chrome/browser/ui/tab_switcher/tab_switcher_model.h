// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_MODEL_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_MODEL_H_

#import <UIKit/UIKit.h>

#include <string>
#include "base/ios/block_types.h"
#import "ios/chrome/browser/sync/synced_sessions_bridge.h"
#import "ios/chrome/browser/tabs/tab_model_observer.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/synced_sessions.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_model_snapshot.h"

class ChromeBrowserState;
@class TabModel;
@class TabSwitcherCache;

enum class TabSwitcherSignInPanelsType {
  PANEL_USER_SIGNED_OUT,
  PANEL_USER_SIGNED_IN_SYNC_OFF,
  PANEL_USER_SIGNED_IN_SYNC_IN_PROGRESS,
  PANEL_USER_SIGNED_IN_SYNC_ON_NO_SESSIONS,
  NO_PANEL,
};

enum class TabSwitcherSessionType {
  OFF_THE_RECORD_SESSION,
  REGULAR_SESSION,
  DISTANT_SESSION
};

// Returns true if the session type is a local session. A local session is a
// "regular session" or an "off the record" session.
bool TabSwitcherSessionTypeIsLocalSession(TabSwitcherSessionType sessionType);

// Protocol to observe changes to the TabSwitcherModel.
@protocol TabSwitcherModelDelegate<NSObject>
// Called when distant sessions were removed or inserted. |removedIndexes| and
// |insertedIndexes| contains indexes in ascending order.
- (void)distantSessionsRemovedAtSortedIndexes:(NSArray*)removedIndexes
                      insertedAtSortedIndexes:(NSArray*)insertedIndexes;
// Called when the distant session with the tag |tag| may need to be updated.
- (void)distantSessionMayNeedUpdate:(std::string const&)tag;
// Called when a local session of type |type| needs to be updated.
- (void)localSessionMayNeedUpdate:(TabSwitcherSessionType)type;
// Called when the signed-in panel (if any) must change.
- (void)signInPanelChangedTo:(TabSwitcherSignInPanelsType)panelType;
// Called to retrieve the size of the item at the |index| in |session|.
- (CGSize)sizeForItemAtIndex:(NSUInteger)index
                   inSession:(TabSwitcherSessionType)session;
// Returns YES if sign-in from Other Devices is in progress.
- (BOOL)isSigninInProgress;
@end

// This class serves as a bridge between Chrome-level data (CLD), and what is
// presented on screen in the Tab Switcher.
// The CLD is:
// -The "distant sessions", i.e. the list of Chrome instances the user is
//  signed-in, and the tabs each of the session contains.
// -The non incognito/incognito tab models. Also known as the "local sessions".
//
// The TabSwitcher observes changes in the CLD, and takes snapshots of its
// state.
// Whenever the delegate can be notified, the TabSwitcher computes a diff
// and notifies the delegate of the changes.
@interface TabSwitcherModel : NSObject<SyncedSessionsObserver, TabModelObserver>

@property(nonatomic, readonly) TabModel* mainTabModel;
@property(nonatomic, readonly) TabModel* otrTabModel;

// Default initializer. |browserState| must not be nullptr, |delegate| must not
// be nil.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                            delegate:(id<TabSwitcherModelDelegate>)delegate
                        mainTabModel:(TabModel*)mainTabModel
                         otrTabModel:(TabModel*)otrTabModel
                           withCache:(TabSwitcherCache*)cache;
// Sets the main and incognito tab models, and notify the delegate of changes,
// if any. |mainTabModel| and |otrTabModel| can be nil.
- (void)setMainTabModel:(TabModel*)mainTabModel
            otrTabModel:(TabModel*)otrTabModel;
// Returns the browserState.
- (ios::ChromeBrowserState*)browserState;
// Returns the latest data for the local session of type |type|.
- (std::unique_ptr<TabModelSnapshot>)tabModelSnapshotForLocalSession:
    (TabSwitcherSessionType)type;
// Returns the latest data for the distant session of tag |tag|.
- (std::unique_ptr<const synced_sessions::DistantSession>)distantSessionForTag:
    (std::string const&)tag;

// Returns the current count of sessions including main and incognito.
- (NSInteger)sessionCount;
// Returns the current count of distant sessions.
- (NSInteger)distantSessionCount;
// Returns the type of the sign in panel.
- (TabSwitcherSignInPanelsType)signInPanelType;
// Returns the tag of the session at index |index|.
- (std::string const&)tagOfDistantSessionAtIndex:(int)index;
// Notifies the delegate that changes occurred for the distant sessions.
- (void)syncedSessionsChanged;
@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_MODEL_H_
