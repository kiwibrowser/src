// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/tab_switcher/tab_switcher_transition_context.h"

#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/ui/browser_view_controller.h"
#include "ios/chrome/browser/ui/tab_switcher/tab_switcher_transition_context.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_snapshot_providing.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@class BrowserViewController;

@interface TabSwitcherTransitionContextContent () {
  UIView<TabStripFoldAnimation>* _tabStripPlaceholderView;
  __weak BrowserViewController* _bvc;
}

@end

@implementation TabSwitcherTransitionContextContent {
}

+ (instancetype)tabSwitcherTransitionContextContentFromBVC:
    (BrowserViewController*)bvc {
  TabSwitcherTransitionContextContent* transitionContextContent =
      [[TabSwitcherTransitionContextContent alloc] init];

  transitionContextContent.initialTabID = bvc.tabModel.currentTab.tabId;

  if (![bvc isViewLoaded]) {
    [bvc loadViewIfNeeded];
    [bvc.view setFrame:[[UIScreen mainScreen] bounds]];
  }

  UIView* toolbarSnapshotView =
      [bvc.toolbarSnapshotProvider snapshotForTabSwitcher];
  transitionContextContent.toolbarSnapshotView = toolbarSnapshotView;
  transitionContextContent->_bvc = bvc;
  return transitionContextContent;
}

- (UIView<TabStripFoldAnimation>*)generateTabStripPlaceholderView {
  return [_bvc tabStripPlaceholderView];
}

@synthesize toolbarSnapshotView = _toolbarSnapshotView;
@synthesize initialTabID = _initialTabID;

- (instancetype)init {
  self = [super init];
  if (self) {
  }
  return self;
}

@end

@implementation TabSwitcherTransitionContext {
}

+ (instancetype)
tabSwitcherTransitionContextWithCurrent:(BrowserViewController*)currentBVC
                                mainBVC:(BrowserViewController*)mainBVC
                                 otrBVC:(BrowserViewController*)otrBVC {
  TabSwitcherTransitionContext* transitionContext =
      [[TabSwitcherTransitionContext alloc] init];
  Tab* currentTab = [[currentBVC tabModel] currentTab];
  if (currentTab) {
    UIImage* tabSnapshotImage =
        SnapshotTabHelper::FromWebState(currentTab.webState)
            ->GenerateSnapshot(/*with_overlays=*/true,
                               /*visible_frame_only=*/true);
    [transitionContext setTabSnapshotImage:tabSnapshotImage];
  }
  [transitionContext
      setIncognitoContent:
          [TabSwitcherTransitionContextContent
              tabSwitcherTransitionContextContentFromBVC:otrBVC]];
  [transitionContext
      setRegularContent:
          [TabSwitcherTransitionContextContent
              tabSwitcherTransitionContextContentFromBVC:mainBVC]];
  [transitionContext setInitialTabModel:currentBVC.tabModel];
  return transitionContext;
}

@synthesize tabSnapshotImage = _tabSnapshotImage;
@synthesize incognitoContent = _incognitoContent;
@synthesize regularContent = _regularContent;
@synthesize initialTabModel = _initialTabModel;

- (instancetype)init {
  self = [super init];
  if (self) {
  }
  return self;
}

@end
