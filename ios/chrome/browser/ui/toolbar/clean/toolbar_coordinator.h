// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/toolbar/public/primary_toolbar_coordinator.h"
#import "ios/chrome/browser/ui/toolbar/public/toolbar_coordinating.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_snapshot_providing.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_presentation_state_provider.h"

@class CommandDispatcher;
@protocol TabHistoryUIUpdater;
@protocol ToolbarCoordinatorDelegate;
@protocol ToolsMenuConfigurationProvider;
@protocol UrlLoader;
class WebStateList;
namespace ios {
class ChromeBrowserState;
}

// Coordinator to run a toolbar -- a UI element housing controls.
@interface ToolbarCoordinator : NSObject<PrimaryToolbarCoordinator,
                                         ToolbarCoordinating,
                                         ToolbarSnapshotProviding,
                                         ToolsMenuPresentationStateProvider>

- (instancetype)
initWithToolsMenuConfigurationProvider:
    (id<ToolsMenuConfigurationProvider>)configurationProvider
                            dispatcher:(CommandDispatcher*)dispatcher
                          browserState:(ios::ChromeBrowserState*)browserState;

- (instancetype)init NS_UNAVAILABLE;

// The web state list this ToolbarCoordinator is handling.
@property(nonatomic, assign) WebStateList* webStateList;
// Delegate for this coordinator. Only used for plumbing to Location Bar
// coordinator.
// TODO(crbug.com/799446): Change this.
@property(nonatomic, weak) id<ToolbarCoordinatorDelegate> delegate;
// URL loader for the toolbar.
// TODO(crbug.com/799446): Remove this.
@property(nonatomic, weak) id<UrlLoader> URLLoader;

- (id<TabHistoryUIUpdater>)tabHistoryUIUpdater;

// Start this coordinator.
- (void)start;
// Stop this coordinator.
- (void)stop;

// Updates the tools menu, changing its content to reflect the current page.
- (void)updateToolsMenu;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_COORDINATOR_H_
