// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/recent_tabs/recent_tabs_coordinator.h"

#include "base/ios/block_types.h"
#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/recent_tabs_handset_view_controller.h"
#import "ios/chrome/browser/ui/recent_tabs/recent_tabs_mediator.h"
#import "ios/chrome/browser/ui/recent_tabs/recent_tabs_table_view_controller.h"
#import "ios/chrome/browser/ui/recent_tabs/recent_tabs_transitioning_delegate.h"
#import "ios/chrome/browser/ui/table_view/table_view_navigation_controller.h"
#import "ios/chrome/browser/ui/table_view/table_view_navigation_controller_constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// TODO(crbug.com/805135): Remove RecentTabsHandsetViewControllerCommand and
// recent_tabs_handset_view_controller.h import. We need this to dismiss for
// now, but it can be improved.
@interface RecentTabsCoordinator ()<RecentTabsHandsetViewControllerCommand>
// Completion block called once the recentTabsViewController is dismissed.
@property(nonatomic, copy) ProceduralBlock completion;
// Mediator being managed by this Coordinator.
@property(nonatomic, strong) RecentTabsMediator* mediator;
// ViewController being managed by this Coordinator.
@property(nonatomic, strong)
    TableViewNavigationController* recentTabsNavigationController;
@property(nonatomic, strong)
    RecentTabsTransitioningDelegate* recentTabsTransitioningDelegate;
@end

@implementation RecentTabsCoordinator
@synthesize completion = _completion;
@synthesize dispatcher = _dispatcher;
@synthesize loader = _loader;
@synthesize mediator = _mediator;
@synthesize recentTabsNavigationController = _recentTabsNavigationController;
@synthesize recentTabsTransitioningDelegate = _recentTabsTransitioningDelegate;

- (void)start {
  // Initialize and configure RecentTabsTableViewController.
  RecentTabsTableViewController* recentTabsTableViewController =
      [[RecentTabsTableViewController alloc] init];
  recentTabsTableViewController.browserState = self.browserState;
  recentTabsTableViewController.loader = self.loader;
  recentTabsTableViewController.dispatcher = self.dispatcher;
  recentTabsTableViewController.handsetCommandHandler = self;

  // Adds the "Done" button and hooks it up to |stop|.
  UIBarButtonItem* dismissButton = [[UIBarButtonItem alloc]
      initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                           target:self
                           action:@selector(stop)];
  [dismissButton
      setAccessibilityIdentifier:kTableViewNavigationDismissButtonId];
  recentTabsTableViewController.navigationItem.rightBarButtonItem =
      dismissButton;

  // Initialize and configure RecentTabsMediator.
  DCHECK(!self.mediator);
  self.mediator = [[RecentTabsMediator alloc] init];
  self.mediator.browserState = self.browserState;
  // Set the consumer first before calling [self.mediator initObservers] and
  // then [self.mediator reloadSessions].
  self.mediator.consumer = recentTabsTableViewController;
  // TODO(crbug.com/845636) : Currently, the image data source must be set
  // before the mediator starts updating its consumer. Fix this so that order of
  // calls does not matter.
  recentTabsTableViewController.imageDataSource = self.mediator;
  recentTabsTableViewController.delegate = self.mediator;
  [self.mediator initObservers];
  [self.mediator reloadSessions];

  // Present RecentTabsNavigationController.
  self.recentTabsNavigationController = [[TableViewNavigationController alloc]
      initWithTable:recentTabsTableViewController];
  self.recentTabsNavigationController.toolbarHidden = YES;
  self.recentTabsTransitioningDelegate =
      [[RecentTabsTransitioningDelegate alloc] init];
  self.recentTabsNavigationController.transitioningDelegate =
      self.recentTabsTransitioningDelegate;
  [self.recentTabsNavigationController
      setModalPresentationStyle:UIModalPresentationCustom];
  [self.baseViewController
      presentViewController:self.recentTabsNavigationController
                   animated:YES
                 completion:nil];
}

- (void)stop {
  // TODO(crbug.com/805135): Create RecentTabsLocalCommands?. Remove
  // "base/mac/foundation_util.h" import then.
  RecentTabsTableViewController* recentTabsTableViewController =
      base::mac::ObjCCastStrict<RecentTabsTableViewController>(
          self.recentTabsNavigationController.tableViewController);
  [recentTabsTableViewController dismissModals];
  [self.recentTabsNavigationController
      dismissViewControllerAnimated:YES
                         completion:self.completion];
  self.recentTabsNavigationController = nil;
  self.recentTabsTransitioningDelegate = nil;
  [self.mediator disconnect];
  self.mediator = nil;
}

#pragma mark - RecentTabsHandsetViewControllerCommand

- (void)dismissRecentTabsWithCompletion:(void (^)())completion {
  self.completion = completion;
  [self stop];
}

@end
