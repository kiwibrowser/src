// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/history/history_coordinator.h"

#include <memory>

#include "components/browser_sync/profile_sync_service.h"
#include "components/history/core/browser/browsing_history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "ios/chrome/browser/experimental_flags.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#import "ios/chrome/browser/ui/commands/application_commands.h"
#include "ios/chrome/browser/ui/history/history_local_commands.h"
#include "ios/chrome/browser/ui/history/history_table_view_controller.h"
#import "ios/chrome/browser/ui/history/history_transitioning_delegate.h"
#include "ios/chrome/browser/ui/history/ios_browsing_history_driver.h"
#import "ios/chrome/browser/ui/settings/clear_browsing_data_coordinator.h"
#import "ios/chrome/browser/ui/table_view/table_view_navigation_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface HistoryCoordinator ()<HistoryLocalCommands> {
  // Provides dependencies and funnels callbacks from BrowsingHistoryService.
  std::unique_ptr<IOSBrowsingHistoryDriver> _browsingHistoryDriver;
  // Abstraction to communicate with HistoryService and WebHistoryService.
  std::unique_ptr<history::BrowsingHistoryService> _browsingHistoryService;
}
// ViewController being managed by this Coordinator.
@property(nonatomic, strong)
    TableViewNavigationController* historyNavigationController;

// The transitioning delegate used by the history view controller.
@property(nonatomic, strong)
    HistoryTransitioningDelegate* historyTransitioningDelegate;

// The coordinator that will present Clear Browsing Data.
@property(nonatomic, strong)
    ClearBrowsingDataCoordinator* clearBrowsingDataCoordinator;
@end

@implementation HistoryCoordinator
@synthesize dispatcher = _dispatcher;
@synthesize historyNavigationController = _historyNavigationController;
@synthesize historyTransitioningDelegate = _historyTransitioningDelegate;
@synthesize loader = _loader;
@synthesize clearBrowsingDataCoordinator = _clearBrowsingDataCoordinator;

- (void)start {
  // Initialize and configure HistoryTableViewController.
  HistoryTableViewController* historyTableViewController =
      [[HistoryTableViewController alloc] init];
  historyTableViewController.browserState = self.browserState;
  historyTableViewController.loader = self.loader;

  // Initialize and configure HistoryServices.
  _browsingHistoryDriver = std::make_unique<IOSBrowsingHistoryDriver>(
      self.browserState, historyTableViewController);
  _browsingHistoryService = std::make_unique<history::BrowsingHistoryService>(
      _browsingHistoryDriver.get(),
      ios::HistoryServiceFactory::GetForBrowserState(
          self.browserState, ServiceAccessType::EXPLICIT_ACCESS),
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(
          self.browserState));
  historyTableViewController.historyService = _browsingHistoryService.get();

  // Configure and present HistoryNavigationController.
  self.historyNavigationController = [[TableViewNavigationController alloc]
      initWithTable:historyTableViewController];
  self.historyNavigationController.toolbarHidden = NO;
  historyTableViewController.localDispatcher = self;
  self.historyTransitioningDelegate =
      [[HistoryTransitioningDelegate alloc] init];
  self.historyNavigationController.transitioningDelegate =
      self.historyTransitioningDelegate;
  [self.historyNavigationController
      setModalPresentationStyle:UIModalPresentationCustom];
  [self.baseViewController
      presentViewController:self.historyNavigationController
                   animated:YES
                 completion:nil];
}

- (void)stop {
  [self stopWithCompletion:nil];
}

- (void)stopWithCompletion:(ProceduralBlock)completionHandler {
  [self.historyNavigationController
      dismissViewControllerAnimated:YES
                         completion:completionHandler];
  self.historyNavigationController = nil;
}

#pragma mark - HistoryLocalCommands

- (void)dismissHistoryWithCompletion:(ProceduralBlock)completionHandler {
  [self stopWithCompletion:completionHandler];
}

- (void)displayPrivacySettings {
  if (experimental_flags::IsCollectionsUIRebootEnabled()) {
    self.clearBrowsingDataCoordinator = [[ClearBrowsingDataCoordinator alloc]
        initWithBaseViewController:self.historyNavigationController
                      browserState:self.browserState];
    [self.clearBrowsingDataCoordinator start];
  } else {
    [self.dispatcher showClearBrowsingDataSettingsFromViewController:
                         self.historyNavigationController];
  }
}

@end
