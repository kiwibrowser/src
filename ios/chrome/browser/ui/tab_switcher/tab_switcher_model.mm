// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_model.h"

#include <memory>

#include "components/browser_sync/profile_sync_service.h"
#include "components/sessions/core/session_id.h"
#include "components/signin/core/browser/signin_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#include "ios/chrome/browser/ui/ntp/recent_tabs/synced_sessions.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_model_snapshot.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_cache.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_model_private.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_cell.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

bool TabSwitcherSessionTypeIsLocalSession(TabSwitcherSessionType sessionType) {
  return sessionType == TabSwitcherSessionType::OFF_THE_RECORD_SESSION ||
         sessionType == TabSwitcherSessionType::REGULAR_SESSION;
}

namespace {

class TagAndIndex {
 public:
  TagAndIndex(std::string const& tag, size_t index)
      : tag_(tag), index_(index) {}
  std::string tag_;
  size_t index_;
  bool operator<(const TagAndIndex& other) const { return tag_ < other.tag_; }
};

void FillSetUsingSessions(synced_sessions::SyncedSessions const& sessions,
                          std::set<TagAndIndex>* set) {
  DCHECK(set);
  DCHECK(set->empty());
  for (size_t i = 0; i < sessions.GetSessionCount(); ++i) {
    set->insert(TagAndIndex(sessions.GetSession(i)->tag, i));
  }
}

}  // namespace

@interface TabSwitcherModel () {
  // The browser state.
  ios::ChromeBrowserState* _browserState;  // weak
  // The tab models.
  __weak TabModel* _mainTabModel;
  __weak TabModel* _otrTabModel;
  // The delegate for event callbacks.
  __weak id<TabSwitcherModelDelegate> _delegate;
  // The synced sessions. Must never be null.
  std::unique_ptr<synced_sessions::SyncedSessions> _syncedSessions;
  // The synced sessions change observer.
  std::unique_ptr<synced_sessions::SyncedSessionsObserverBridge>
      _syncedSessionsObserver;
  // Snapshots of the |_mainTabModel| and |_otrTabModel|.
  std::unique_ptr<TabModelSnapshot> _mainTabModelSnapshot;
  std::unique_ptr<TabModelSnapshot> _otrTabModelSnapshot;
  // The cache holding resized tabs snapshots.
  TabSwitcherCache* _cache;
}

// Returns the type of the local session corresponding to the given |tabModel|.
// |tabModel| MUST be equal to either |_mainTabModel|, or |_otrTabModel|.
- (TabSwitcherSessionType)typeOfLocalSessionForTabModel:(TabModel*)tabModel;
@end

@implementation TabSwitcherModel

@synthesize mainTabModel = _mainTabModel;
@synthesize otrTabModel = _otrTabModel;

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                            delegate:(id<TabSwitcherModelDelegate>)delegate
                        mainTabModel:(TabModel*)mainTabModel
                         otrTabModel:(TabModel*)otrTabModel
                           withCache:(TabSwitcherCache*)cache {
  DCHECK(browserState);
  DCHECK(delegate);
  // TODO(jif): DCHECK |mainTabModel| and |otrTabModel|.
  self = [self init];
  if (self) {
    _browserState = browserState;
    _delegate = delegate;
    _syncedSessions.reset(new synced_sessions::SyncedSessions());
    _syncedSessionsObserver.reset(
        new synced_sessions::SyncedSessionsObserverBridge(self, _browserState));
    _mainTabModel = mainTabModel;
    _otrTabModel = otrTabModel;
    _mainTabModelSnapshot.reset(new TabModelSnapshot(mainTabModel));
    _otrTabModelSnapshot.reset(new TabModelSnapshot(otrTabModel));
    [_mainTabModel addObserver:self];
    [_otrTabModel addObserver:self];
    _cache = cache;
  }
  return self;
}

- (void)setMainTabModel:(TabModel*)mainTabModel
            otrTabModel:(TabModel*)otrTabModel {
  [self replaceMainTabModelWithTabModel:mainTabModel];
  [self replaceOTRTabModelWithTabModel:otrTabModel];
}

- (void)replaceMainTabModelWithTabModel:(TabModel*)newTabModel {
  if (_mainTabModel == newTabModel)
    return;
  [_mainTabModel removeObserver:self];
  _mainTabModel = newTabModel;
  [newTabModel addObserver:self];
  // Calling |tabModelChanged:| may trigger an animated refresh of the
  // Tab Switcher's collection view.
  // Here in |replaceOldTabModel:withTabModel:| the animation is undesirable.
  [UIView performWithoutAnimation:^{
    [self tabModelChanged:newTabModel];
  }];
}

- (void)replaceOTRTabModelWithTabModel:(TabModel*)newTabModel {
  if (_otrTabModel == newTabModel)
    return;
  [_otrTabModel removeObserver:self];
  _otrTabModel = newTabModel;
  [newTabModel addObserver:self];
  // Calling |tabModelChanged:| may trigger an animated refresh of the
  // Tab Switcher's collection view.
  // Here in |replaceOldTabModel:withTabModel:| the animation is undesirable.
  [UIView performWithoutAnimation:^{
    [self tabModelChanged:newTabModel];
  }];
}

- (void)dealloc {
  [_mainTabModel removeObserver:self];
  [_otrTabModel removeObserver:self];
}

- (NSInteger)sessionCount {
  const NSInteger mainTabModelSessionCount = 1;
  const NSInteger otrTabModelSessionCount = 1;
  return mainTabModelSessionCount + otrTabModelSessionCount +
         [self distantSessionCount];
}

- (NSInteger)distantSessionCount {
  return _syncedSessions->GetSessionCount();
}

- (ios::ChromeBrowserState*)browserState {
  return _browserState;
}

- (TabModel*)tabModelForSessionOfType:(TabSwitcherSessionType)type {
  DCHECK(type == TabSwitcherSessionType::OFF_THE_RECORD_SESSION ||
         type == TabSwitcherSessionType::REGULAR_SESSION);
  return type == TabSwitcherSessionType::OFF_THE_RECORD_SESSION ? _otrTabModel
                                                                : _mainTabModel;
}

- (NSInteger)numberOfTabsInLocalSessionOfType:(TabSwitcherSessionType)type {
  TabModelSnapshot* tabModelSnapshot = [self tabModelSnapshotForSession:type];
  return tabModelSnapshot->tabs().size();
}

- (Tab*)tabAtIndex:(NSUInteger)index
    inLocalSessionOfType:(TabSwitcherSessionType)type {
  TabModelSnapshot* tabModelSnapshot = [self tabModelSnapshotForSession:type];
  return tabModelSnapshot->tabs()[index];
}

- (std::unique_ptr<TabModelSnapshot>)tabModelSnapshotForLocalSession:
    (TabSwitcherSessionType)type {
  TabModel* tm = nullptr;
  switch (type) {
    case TabSwitcherSessionType::OFF_THE_RECORD_SESSION:
      tm = _otrTabModel;
      break;
    case TabSwitcherSessionType::REGULAR_SESSION:
      tm = _mainTabModel;
      break;
    default:
      NOTREACHED();
      break;
  }
  return std::make_unique<TabModelSnapshot>(tm);
}

- (std::unique_ptr<const synced_sessions::DistantSession>)distantSessionForTag:
    (std::string const&)tag {
  syncer::SyncService* syncService =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(_browserState);
  return std::make_unique<synced_sessions::DistantSession>(syncService, tag);
}

- (std::string const&)tagOfDistantSessionAtIndex:(int)index {
  return _syncedSessions->GetSession(index)->tag;
}

- (TabSwitcherSignInPanelsType)signInPanelType {
  if (![self isSignedIn] || [_delegate isSigninInProgress])
    return TabSwitcherSignInPanelsType::PANEL_USER_SIGNED_OUT;
  if (![self isSyncTabsEnabled])
    return TabSwitcherSignInPanelsType::PANEL_USER_SIGNED_IN_SYNC_OFF;
  if (_syncedSessions->GetSessionCount() != 0)
    return TabSwitcherSignInPanelsType::NO_PANEL;
  if ([self isFirstSyncCycleCompleted]) {
    return TabSwitcherSignInPanelsType::
        PANEL_USER_SIGNED_IN_SYNC_ON_NO_SESSIONS;
  } else {
    return TabSwitcherSignInPanelsType::PANEL_USER_SIGNED_IN_SYNC_IN_PROGRESS;
  }
}

- (void)syncedSessionsChanged {
  syncer::SyncService* syncService =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(_browserState);

  std::unique_ptr<synced_sessions::SyncedSessions> oldSyncedSessions(
      new synced_sessions::SyncedSessions(syncService));
  _syncedSessions.swap(oldSyncedSessions);

  // Notify about change in the sign in panel.
  TabSwitcherSignInPanelsType panelType = [self signInPanelType];
  [_delegate signInPanelChangedTo:panelType];

  if (panelType != TabSwitcherSignInPanelsType::NO_PANEL) {
    // Do not show synced sessions.
    _syncedSessions.reset(new synced_sessions::SyncedSessions());
  }

  // Notify about changes in the synced sessions.
  [TabSwitcherModel notifyDelegate:_delegate
                   aboutChangeFrom:*oldSyncedSessions
                                to:*_syncedSessions];
}

- (BOOL)isSignedIn {
  SigninManager* signin_manager =
      ios::SigninManagerFactory::GetForBrowserState(_browserState);
  return signin_manager->IsAuthenticated();
}

- (BOOL)isSyncTabsEnabled {
  DCHECK([self isSignedIn]);
  SyncSetupService* service =
      SyncSetupServiceFactory::GetForBrowserState(_browserState);
  return !service->UserActionIsRequiredToHaveSyncWork();
}

- (BOOL)isFirstSyncCycleCompleted {
  return _syncedSessionsObserver->IsFirstSyncCycleCompleted();
}

+ (void)notifyDelegate:(id<TabSwitcherModelDelegate>)delegate
       aboutChangeFrom:(synced_sessions::SyncedSessions&)oldSessions
                    to:(synced_sessions::SyncedSessions&)newSessions {
  // Compute and notify the delegate about removed or inserted sessions.
  std::set<TagAndIndex> tagsOfOldSessions;
  std::set<TagAndIndex> tagsOfNewSessions;
  FillSetUsingSessions(oldSessions, &tagsOfOldSessions);
  FillSetUsingSessions(newSessions, &tagsOfNewSessions);

  std::set<TagAndIndex> tagsOfRemovedSessions;
  std::set<TagAndIndex> tagsOfInsertedSessions;
  std::set_difference(
      tagsOfOldSessions.begin(), tagsOfOldSessions.end(),
      tagsOfNewSessions.begin(), tagsOfNewSessions.end(),
      std::inserter(tagsOfRemovedSessions, tagsOfRemovedSessions.end()));
  std::set_difference(
      tagsOfNewSessions.begin(), tagsOfNewSessions.end(),
      tagsOfOldSessions.begin(), tagsOfOldSessions.end(),
      std::inserter(tagsOfInsertedSessions, tagsOfInsertedSessions.end()));

  NSArray* removedIndexesSorted = nil;
  NSArray* insertedIndexesSorted = nil;
  if (!tagsOfRemovedSessions.empty()) {
    NSMutableArray* removedIndexes = [NSMutableArray array];
    for (auto& tagAndIndex : tagsOfRemovedSessions) {
      [removedIndexes addObject:[NSNumber numberWithInt:tagAndIndex.index_]];
    }
    removedIndexesSorted =
        [removedIndexes sortedArrayUsingSelector:@selector(compare:)];
  }
  if (!tagsOfInsertedSessions.empty()) {
    NSMutableArray* insertedIndexes = [NSMutableArray array];
    for (auto& tagAndIndex : tagsOfInsertedSessions) {
      [insertedIndexes addObject:[NSNumber numberWithInt:tagAndIndex.index_]];
    }
    insertedIndexesSorted =
        [insertedIndexes sortedArrayUsingSelector:@selector(compare:)];
  }
  if (removedIndexesSorted || insertedIndexesSorted) {
    [delegate distantSessionsRemovedAtSortedIndexes:removedIndexesSorted
                            insertedAtSortedIndexes:insertedIndexesSorted];
  }
  // Compute and notify the delegate about tabs that were removed or inserted
  // in the sessions that weren't inserted or deleted.
  std::set<TagAndIndex> tagsOfOtherSessions;
  std::set_intersection(
      tagsOfNewSessions.begin(), tagsOfNewSessions.end(),
      tagsOfOldSessions.begin(), tagsOfOldSessions.end(),
      std::inserter(tagsOfOtherSessions, tagsOfOtherSessions.end()));
  for (TagAndIndex const& tagAndIndexOfSession : tagsOfOtherSessions) {
    [delegate distantSessionMayNeedUpdate:tagAndIndexOfSession.tag_];
  }
}

- (TabSwitcherSessionType)typeOfLocalSessionForTabModel:(TabModel*)tabModel {
  DCHECK(tabModel == _mainTabModel || tabModel == _otrTabModel);
  if (tabModel == _otrTabModel)
    return TabSwitcherSessionType::OFF_THE_RECORD_SESSION;
  return TabSwitcherSessionType::REGULAR_SESSION;
}

- (TabModelSnapshot*)tabModelSnapshotForSession:(TabSwitcherSessionType)type {
  switch (type) {
    case TabSwitcherSessionType::OFF_THE_RECORD_SESSION:
      return _otrTabModelSnapshot.get();
    case TabSwitcherSessionType::REGULAR_SESSION:
      return _mainTabModelSnapshot.get();
    default:
      NOTREACHED();
      return nullptr;
      break;
  }
}

- (void)setTabModelSnapshot:(std::unique_ptr<TabModelSnapshot>)tabModelSnapshot
                 forSession:(TabSwitcherSessionType)type {
  switch (type) {
    case TabSwitcherSessionType::OFF_THE_RECORD_SESSION:
      _otrTabModelSnapshot = std::move(tabModelSnapshot);
      break;
    case TabSwitcherSessionType::REGULAR_SESSION:
      _mainTabModelSnapshot = std::move(tabModelSnapshot);
      break;
    default:
      NOTREACHED();
      break;
  }
}

- (void)tabModelChanged:(TabModel*)tabModel {
  TabSwitcherSessionType sessionType =
      [self typeOfLocalSessionForTabModel:tabModel];
  [_delegate localSessionMayNeedUpdate:sessionType];
}

#pragma mark - SyncedSessionsObserver

- (void)reloadSessions {
  [self syncedSessionsChanged];
}

- (void)onSyncStateChanged {
  [self syncedSessionsChanged];
}

#pragma mark - TabModelObserver

- (void)tabModel:(TabModel*)model didChangeTab:(Tab*)tab {
  [self tabModelChanged:model];
}

- (void)tabModel:(TabModel*)model
    didInsertTab:(Tab*)tab
         atIndex:(NSUInteger)index
    inForeground:(BOOL)fg {
  [self tabModelChanged:model];
}

- (void)tabModel:(TabModel*)model
    didRemoveTab:(Tab*)tab
         atIndex:(NSUInteger)index {
  [self tabModelChanged:model];
}

- (void)tabModel:(TabModel*)model
      didMoveTab:(Tab*)tab
       fromIndex:(NSUInteger)fromIndex
         toIndex:(NSUInteger)toIndex {
  [self tabModelChanged:model];
}

- (void)tabModel:(TabModel*)model
    didReplaceTab:(Tab*)oldTab
          withTab:(Tab*)newTab
          atIndex:(NSUInteger)index {
  [self tabModelChanged:model];
}

- (void)tabModel:(TabModel*)model
    didChangeTabSnapshot:(Tab*)tab
               withImage:(UIImage*)image {
  [self tabModelChanged:model];
}

@end
