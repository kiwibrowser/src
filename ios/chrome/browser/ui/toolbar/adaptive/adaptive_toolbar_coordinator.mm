// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/adaptive/adaptive_toolbar_coordinator.h"

#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/reading_list/reading_list_model_factory.h"
#import "ios/chrome/browser/ui/ntp/ntp_util.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/adaptive_toolbar_coordinator+subclassing.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/adaptive_toolbar_view_controller.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_factory.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_visibility_configuration.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_tools_menu_button.h"
#import "ios/chrome/browser/ui/toolbar/buttons/tools_menu_button_observer_bridge.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_mediator.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/public/provider/chrome/browser/chrome_browser_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface AdaptiveToolbarCoordinator ()

// Whether this coordinator has been started.
@property(nonatomic, assign) BOOL started;
// Mediator for updating the toolbar when the WebState changes.
@property(nonatomic, strong) ToolbarMediator* mediator;
// Button observer for the ToolsMenu button.
@property(nonatomic, strong)
    ToolsMenuButtonObserverBridge* toolsMenuButtonObserverBridge;

@end

@implementation AdaptiveToolbarCoordinator
@synthesize dispatcher = _dispatcher;
@synthesize mediator = _mediator;
@synthesize started = _started;
@synthesize toolsMenuButtonObserverBridge = _toolsMenuButtonObserverBridge;
@synthesize viewController = _viewController;
@synthesize webStateList = _webStateList;

#pragma mark - ChromeCoordinator

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState {
  return [super initWithBaseViewController:nil browserState:browserState];
}

- (void)start {
  if (self.started)
    return;

  self.started = YES;

  self.mediator = [[ToolbarMediator alloc] init];
  self.mediator.consumer = self.viewController;
  self.mediator.webStateList = self.webStateList;
  self.mediator.bookmarkModel =
      ios::BookmarkModelFactory::GetForBrowserState(self.browserState);

  DCHECK(self.viewController.toolsMenuButton);
  self.toolsMenuButtonObserverBridge = [[ToolsMenuButtonObserverBridge alloc]
      initWithModel:ReadingListModelFactory::GetForBrowserState(
                        self.browserState)
      toolbarButton:self.viewController.toolsMenuButton];
}

- (void)stop {
  [super stop];
  self.toolsMenuButtonObserverBridge = nil;
  [self.mediator disconnect];
  self.mediator = nil;
}

#pragma mark - SideSwipeToolbarSnapshotProviding

- (UIImage*)toolbarSideSwipeSnapshotForWebState:(web::WebState*)webState {
  [self updateToolbarForSideSwipeSnapshot:webState];

  UIImage* toolbarSnapshot = CaptureViewWithOption(
      [self.viewController view], [[UIScreen mainScreen] scale],
      kClientSideRendering);

  [self resetToolbarAfterSideSwipeSnapshot];

  return toolbarSnapshot;
}

#pragma mark - NewTabPageControllerDelegate

- (void)setToolbarBackgroundToIncognitoNTPColorWithAlpha:(CGFloat)alpha {
  // TODO(crbug.com/803379): Implement that.
}

- (void)setScrollProgressForTabletOmnibox:(CGFloat)progress {
  [self.viewController setScrollProgressForTabletOmnibox:progress];
}

#pragma mark - ToolbarCommands

- (void)triggerToolsMenuButtonAnimation {
  [self.viewController.toolsMenuButton triggerAnimation];
}

#pragma mark - ToolbarCoordinatee

- (id<PopupMenuUIUpdating>)popupMenuUIUpdater {
  return self.viewController;
}

#pragma mark - Protected

- (ToolbarButtonFactory*)buttonFactoryWithType:(ToolbarType)type {
  BOOL isIncognito = self.browserState->IsOffTheRecord();
  ToolbarStyle style = isIncognito ? INCOGNITO : NORMAL;

  ToolbarButtonFactory* buttonFactory =
      [[ToolbarButtonFactory alloc] initWithStyle:style];
  buttonFactory.dispatcher = self.dispatcher;
  buttonFactory.visibilityConfiguration =
      [[ToolbarButtonVisibilityConfiguration alloc] initWithType:type];

  return buttonFactory;
}

- (void)updateToolbarForSideSwipeSnapshot:(web::WebState*)webState {
  BOOL isNTP = IsVisibleUrlNewTabPage(webState);

  [self.mediator updateConsumerForWebState:webState];
  [self.viewController updateForSideSwipeSnapshotOnNTP:isNTP];
}

- (void)resetToolbarAfterSideSwipeSnapshot {
  [self.mediator
      updateConsumerForWebState:self.webStateList->GetActiveWebState()];
  [self.viewController resetAfterSideSwipeSnapshot];
}

@end
