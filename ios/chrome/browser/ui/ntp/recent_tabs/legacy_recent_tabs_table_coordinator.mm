// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/recent_tabs/legacy_recent_tabs_table_coordinator.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/legacy_recent_tabs_table_view_controller.h"
#import "ios/chrome/browser/ui/recent_tabs/recent_tabs_mediator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface LegacyRecentTabsTableCoordinator () {
  LegacyRecentTabsTableViewController* _tableViewController;
}

@property(nonatomic, strong) RecentTabsMediator* mediator;

@end

@implementation LegacyRecentTabsTableCoordinator
@synthesize handsetCommandHandler = _handsetCommandHandler;
@synthesize mediator = _mediator;

- (instancetype)initWithLoader:(id<UrlLoader>)loader
                  browserState:(ios::ChromeBrowserState*)browserState
                    dispatcher:(id<ApplicationCommands>)dispatcher {
  return [self initWithController:[[LegacyRecentTabsTableViewController alloc]
                                      initWithBrowserState:browserState
                                                    loader:loader
                                                dispatcher:dispatcher]
                     browserState:browserState];
}

- (instancetype)initWithController:
                    (LegacyRecentTabsTableViewController*)controller
                      browserState:(ios::ChromeBrowserState*)browserState {
  self = [super initWithBaseViewController:nil browserState:browserState];
  if (self) {
    DCHECK(controller);
    DCHECK(browserState);
    _tableViewController = controller;
    _mediator = [[RecentTabsMediator alloc] init];
    _mediator.browserState = browserState;
    _mediator.consumer = _tableViewController;
    [_tableViewController setDelegate:_mediator];
  }
  return self;
}

- (void)start {
  _tableViewController.handsetCommandHandler = self.handsetCommandHandler;
  [self.mediator initObservers];
  [self.mediator reloadSessions];
}

- (void)stop {
  [_tableViewController dismissModals];
  [_tableViewController setDelegate:nil];
  [self.mediator disconnect];
}

- (UIViewController*)viewController {
  return _tableViewController;
}

@end
