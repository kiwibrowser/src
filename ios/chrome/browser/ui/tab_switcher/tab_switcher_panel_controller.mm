// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_controller.h"

#include "base/logging.h"
#import "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/tabs/tab.h"
#include "ios/chrome/browser/ui/ntp/recent_tabs/synced_sessions.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_cache.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_cell.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_overlay_view.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_view.h"
#include "ios/chrome/browser/ui/tab_switcher/tab_switcher_session_changes.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

void FillVectorWithHashesUsingDistantSession(
    synced_sessions::DistantSession const& session,
    std::vector<size_t>* hashes) {
  DCHECK(hashes);
  DCHECK(hashes->empty());
  for (size_t i = 0; i < session.tabs.size(); ++i) {
    hashes->push_back(session.tabs[i]->hashOfUserVisibleProperties());
  }
}

}  // namespace

@interface TabSwitcherPanelController ()<UICollectionViewDataSource,
                                         UICollectionViewDelegate,
                                         SessionCellDelegate> {
  ios::ChromeBrowserState* _browserState;  // Weak.
  TabSwitcherPanelView* _panelView;
  TabSwitcherModel* _model;
  std::string _sessionTag;
  TabSwitcherSessionType _sessionType;
  TabSwitcherCache* _cache;
  TabSwitcherPanelOverlayView* _overlayView;
  std::unique_ptr<const synced_sessions::DistantSession> _distantSession;
  std::unique_ptr<const TabModelSnapshot> _localSession;
}

// Changes the visibility of the zero tab state overlay view.
- (void)setZeroTabStateOverlayVisible:(BOOL)show;

@end

@implementation TabSwitcherPanelController

@synthesize delegate = _delegate;
@synthesize sessionType = _sessionType;
@synthesize presenter = _presenter;
@synthesize dispatcher = _dispatcher;

- (instancetype)initWithModel:(TabSwitcherModel*)model
     forDistantSessionWithTag:(std::string const&)sessionTag
                 browserState:(ios::ChromeBrowserState*)browserState
                    presenter:(id<SigninPresenter, SyncPresenter>)presenter
                   dispatcher:
                       (id<ApplicationCommands, BrowserCommands>)dispatcher {
  self = [super init];
  if (self) {
    DCHECK(model);
    _presenter = presenter;
    _dispatcher = dispatcher;
    _sessionType = TabSwitcherSessionType::DISTANT_SESSION;
    _model = model;
    _distantSession = [model distantSessionForTag:sessionTag];
    _sessionTag = sessionTag;
    _browserState = browserState;
    [self loadView];
  }
  return self;
}

- (instancetype)initWithModel:(TabSwitcherModel*)model
        forLocalSessionOfType:(TabSwitcherSessionType)sessionType
                    withCache:(TabSwitcherCache*)cache
                 browserState:(ios::ChromeBrowserState*)browserState
                    presenter:(id<SigninPresenter, SyncPresenter>)presenter
                   dispatcher:
                       (id<ApplicationCommands, BrowserCommands>)dispatcher {
  self = [super init];
  if (self) {
    DCHECK(model);
    _presenter = presenter;
    _dispatcher = dispatcher;
    _sessionType = sessionType;
    _model = model;
    _localSession = [model tabModelSnapshotForLocalSession:sessionType];
    _cache = cache;
    _browserState = browserState;
    [self loadView];
  }
  return self;
}

- (TabSwitcherPanelView*)view {
  return _panelView;
}

- (std::string)sessionTag {
  return _sessionTag;
}

- (void)setDelegate:(id<TabSwitcherPanelControllerDelegate>)delegate {
  _delegate = delegate;
  [[_panelView collectionView] performBatchUpdates:nil completion:nil];
}

- (BOOL)shouldShowNewTabButton {
  if (_sessionType == TabSwitcherSessionType::DISTANT_SESSION) {
    return NO;
  } else {
    return ![self isOverlayVisible];
  }
}

- (void)updateCollectionViewIfNeeded {
  if (_sessionType == TabSwitcherSessionType::DISTANT_SESSION) {
    UICollectionView* collectionView = [_panelView collectionView];
    // TODO(crbug.com/633928) Compute TabSwitcherSessionChanges outside of the
    // updateBlock.
    auto updateBlock = ^{
      std::unique_ptr<const synced_sessions::DistantSession> newDistantSession =
          [_model distantSessionForTag:_sessionTag];
      std::vector<size_t> oldTabsHashes;
      std::vector<size_t> newTabsHashes;
      FillVectorWithHashesUsingDistantSession(*_distantSession.get(),
                                              &oldTabsHashes);
      FillVectorWithHashesUsingDistantSession(*newDistantSession.get(),
                                              &newTabsHashes);
      TabSwitcherSessionChanges changes(oldTabsHashes, newTabsHashes);
      if (changes.HasChanges()) {
        _distantSession = std::move(newDistantSession);
        [self applyChanges:changes toCollectionView:collectionView];
      }
    };
    [collectionView performBatchUpdates:updateBlock completion:nil];
  } else {
    UICollectionView* collectionView = [_panelView collectionView];
    auto updateBlock = ^{
      std::unique_ptr<const TabModelSnapshot> newLocalSession =
          [_model tabModelSnapshotForLocalSession:_sessionType];
      TabSwitcherSessionChanges changes(_localSession->hashes(),
                                        newLocalSession->hashes());
      if (changes.HasChanges()) {
        _localSession = std::move(newLocalSession);
        [self applyChanges:changes toCollectionView:collectionView];
      }
    };
    [collectionView performBatchUpdates:updateBlock completion:nil];
  }
}

- (void)applyChanges:(TabSwitcherSessionChanges&)changes
    toCollectionView:(UICollectionView*)collectionView {
  NSMutableArray* deletedIndexes = [NSMutableArray array];
  NSMutableArray* insertedIndexes = [NSMutableArray array];
  NSMutableArray* updatedIndexes = [NSMutableArray array];
  for (size_t i : changes.deletions()) {
    NSInteger deletedTabIndex = static_cast<NSInteger>(i);
    [deletedIndexes
        addObject:[NSIndexPath indexPathForItem:deletedTabIndex inSection:0]];
  }
  for (size_t i : changes.insertions()) {
    NSInteger insertedTabIndex = static_cast<NSInteger>(i);
    [insertedIndexes
        addObject:[NSIndexPath indexPathForItem:insertedTabIndex inSection:0]];
  }
  for (size_t i : changes.updates()) {
    NSInteger updatedTabIndex = static_cast<NSInteger>(i);
    [updatedIndexes
        addObject:[NSIndexPath indexPathForItem:updatedTabIndex inSection:0]];
  }
  [collectionView deleteItemsAtIndexPaths:deletedIndexes];
  [collectionView insertItemsAtIndexPaths:insertedIndexes];
  [collectionView reloadItemsAtIndexPaths:updatedIndexes];
}

- (void)scrollTabIndexToVisible:(NSInteger)index triggerLayout:(BOOL)layout {
  [[_panelView collectionView]
      scrollToItemAtIndexPath:[NSIndexPath indexPathForItem:index inSection:0]
             atScrollPosition:UICollectionViewScrollPositionTop
                     animated:NO];
  if (layout)
    [[_panelView collectionView] layoutIfNeeded];
}

- (TabSwitcherLocalSessionCell*)localSessionCellForTabAtIndex:(NSInteger)index {
  return (TabSwitcherLocalSessionCell*)[[_panelView collectionView]
      cellForItemAtIndexPath:[NSIndexPath indexPathForItem:index inSection:0]];
}

- (CGRect)localSessionCellFrameForTabAtIndex:(NSInteger)index {
  UICollectionViewLayoutAttributes* attributes = [[_panelView collectionView]
      layoutAttributesForItemAtIndexPath:[NSIndexPath indexPathForItem:index
                                                             inSection:0]];
  return attributes.frame;
}

- (void)reload {
  [[_panelView collectionView] reloadSections:[NSIndexSet indexSetWithIndex:0]];
}

- (UICollectionView*)collectionView {
  return [_panelView collectionView];
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)collectionView:(UICollectionView*)collectionView
     numberOfItemsInSection:(NSInteger)section {
  DCHECK_EQ(section, 0);
  if (_sessionType == TabSwitcherSessionType::DISTANT_SESSION) {
    CHECK(_distantSession);
    return _distantSession->tabs.size();
  } else {
    CHECK(_localSession);
    NSInteger numberOfTabs = _localSession->tabs().size();
    [self setZeroTabStateOverlayVisible:numberOfTabs == 0];
    return numberOfTabs;
  }
}

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(NSIndexPath*)indexPath {
  TabSwitcherSessionCell* cell = nil;
  NSUInteger tabIndex = indexPath.item;
  if (_sessionType == TabSwitcherSessionType::DISTANT_SESSION) {
    NSString* identifier = [TabSwitcherDistantSessionCell identifier];
    TabSwitcherDistantSessionCell* panelCell =
        base::mac::ObjCCastStrict<TabSwitcherDistantSessionCell>([collectionView
            dequeueReusableCellWithReuseIdentifier:identifier
                                      forIndexPath:indexPath]);
    cell = panelCell;

    CHECK(_distantSession);
    const std::size_t distantSessionTabCount = _distantSession->tabs.size();
    LOG_ASSERT(tabIndex < distantSessionTabCount)
        << "tabIndex == " << tabIndex
        << " _distantSession->tabs.size() == " << distantSessionTabCount;
    synced_sessions::DistantTab* tab = _distantSession->tabs[tabIndex].get();
    CHECK(tab);
    [panelCell setTitle:base::SysUTF16ToNSString(tab->title)];
    [panelCell setSessionGURL:tab->virtual_url
             withBrowserState:[_model browserState]];
    [panelCell setDelegate:self];
  } else {
    NSString* identifier = [TabSwitcherLocalSessionCell identifier];
    TabSwitcherLocalSessionCell* panelCell =
        base::mac::ObjCCastStrict<TabSwitcherLocalSessionCell>([collectionView
            dequeueReusableCellWithReuseIdentifier:identifier
                                      forIndexPath:indexPath]);
    cell = panelCell;

    Tab* tab = _localSession->tabs()[tabIndex];
    [panelCell setSessionType:_sessionType];
    [panelCell setDelegate:self];
    [panelCell setAppearanceForTab:tab cellSize:[_panelView cellSize]];
  }
  DCHECK(cell);
  return cell;
}

- (void)collectionView:(UICollectionView*)collectionView
    didEndDisplayingCell:(UICollectionViewCell*)cell
      forItemAtIndexPath:(NSIndexPath*)indexPath {
  // When closing the last tab, accessibility does not realize that there is no
  // elements in the collection view and keep the custom action for the tab. So
  // reset the delegate of the cell (to avoid crashing if the action is invoked)
  // and post a notification that the screen changed to force accessibility to
  // re-inspect the whole screen. See http://crbug.com/677374 for crash log.
  base::mac::ObjCCastStrict<TabSwitcherSessionCell>(cell).delegate = nil;
  UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification,
                                  nil);
}

#pragma mark - SessionCellDelegate

- (TabSwitcherCache*)tabSwitcherCache {
  return _cache;
}

- (void)cellPressed:(UICollectionViewCell*)cell {
  const NSInteger tabIndex =
      [[_panelView collectionView] indexPathForCell:cell].item;

  if (_sessionType == TabSwitcherSessionType::DISTANT_SESSION) {
    synced_sessions::DistantTab* tab = _distantSession->tabs[tabIndex].get();
    if (tab)
      [self.delegate tabSwitcherPanelController:self didSelectDistantTab:tab];
  } else {
    Tab* tab = _localSession->tabs()[tabIndex];
    if (tab)
      [self.delegate tabSwitcherPanelController:self didSelectLocalTab:tab];
  }
}

- (void)deleteButtonPressedForCell:(UICollectionViewCell*)cell {
  DCHECK(_sessionType != TabSwitcherSessionType::DISTANT_SESSION);
  const NSInteger tabIndex =
      [[_panelView collectionView] indexPathForCell:cell].item;
  Tab* tab = _localSession->tabs()[tabIndex];
  if (tab)
    [self.delegate tabSwitcherPanelController:self didCloseLocalTab:tab];
}

#pragma mark - Private

- (BOOL)isOverlayVisible {
  return _overlayView && [_overlayView alpha] != 0.0;
}

- (void)setZeroTabStateOverlayVisible:(BOOL)show {
  if (show == [self isOverlayVisible])
    return;

  DCHECK(TabSwitcherSessionTypeIsLocalSession(_sessionType));

  if (!_overlayView) {
    _overlayView = [[TabSwitcherPanelOverlayView alloc]
        initWithFrame:[_panelView bounds]
         browserState:_browserState
            presenter:self.presenter /* id<SigninPresenter, SyncPresenter> */
           dispatcher:self.dispatcher];
    [_overlayView
        setOverlayType:
            (_sessionType == TabSwitcherSessionType::OFF_THE_RECORD_SESSION)
                ? TabSwitcherPanelOverlayType::
                      OVERLAY_PANEL_USER_NO_INCOGNITO_TABS
                : TabSwitcherPanelOverlayType::OVERLAY_PANEL_USER_NO_OPEN_TABS];

    [_overlayView setAlpha:0];
    [_overlayView setAutoresizingMask:UIViewAutoresizingFlexibleHeight |
                                      UIViewAutoresizingFlexibleWidth];
    [_panelView addSubview:_overlayView];
    [_overlayView setNeedsLayout];
  }

  [UIView
      animateWithDuration:0.25
               animations:^{
                 [_overlayView setAlpha:show ? 1.0 : 0.0];
                 [self.delegate
                     tabSwitcherPanelControllerDidUpdateOverlayViewVisibility:
                         self];
               }];
}

- (void)loadView {
  _panelView = [[TabSwitcherPanelView alloc] initWithSessionType:_sessionType];
  _panelView.collectionView.dataSource = self;
  _panelView.collectionView.delegate = self;
}

- (synced_sessions::DistantSession const*)distantSession {
  return _distantSession.get();
}

@end
