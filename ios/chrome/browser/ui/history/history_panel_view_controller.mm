// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/history_panel_view_controller.h"

#include <memory>

#include "base/ios/block_types.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/history/core/browser/browsing_history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#import "ios/chrome/browser/ui/commands/application_commands.h"
#import "ios/chrome/browser/ui/history/clear_browsing_bar.h"
#import "ios/chrome/browser/ui/history/history_search_view_controller.h"
#include "ios/chrome/browser/ui/history/ios_browsing_history_driver.h"
#import "ios/chrome/browser/ui/history/legacy_history_collection_view_controller.h"
#import "ios/chrome/browser/ui/icons/chrome_icon.h"
#import "ios/chrome/browser/ui/keyboard/UIKeyCommand+Chrome.h"
#import "ios/chrome/browser/ui/material_components/utils.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/panel_bar_view.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/url_loader.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/AppBar/src/MaterialAppBar.h"
#import "ios/third_party/material_components_ios/src/components/NavigationBar/src/MaterialNavigationBar.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Shadow opacity for the clear browsing button and the header when scrolling.
CGFloat kShadowOpacity = 0.2f;
}  // namespace

@interface HistoryPanelViewController ()<
    LegacyHistoryCollectionViewControllerDelegate,
    HistorySearchViewControllerDelegate> {
  // Controller for collection view that displays history entries.
  LegacyHistoryCollectionViewController* _historyCollectionController;
  // Bar at the bottom of the history panel the displays options for entry
  // deletion, including "Clear Browsing Data..." which takes the user to
  // Privacy settings, or "Edit" for entering a mode for deleting individual
  // entries. When in edit mode, the bar displays options to Delete or Cancel.
  ClearBrowsingBar* _clearBrowsingBar;
  // View controller for the search bar.
  HistorySearchViewController* _searchViewController;
  // Container view for history collection and clear browsing button to enable
  // use of autolayout in conjunction with Material App Bar.
  UIView* _containerView;
  // The header view.
  MDCAppBar* _appBar;
  // Left bar button item for Search.
  UIBarButtonItem* _leftBarButtonItem;
  // Right bar button item for Dismiss history action.
  UIBarButtonItem* _rightBarButtonItem;
  // YES if NSLayoutConstraits were added.
  BOOL _addedConstraints;
  // Provides dependencies and funnels callbacks from BrowsingHistoryService.
  std::unique_ptr<IOSBrowsingHistoryDriver> _browsingHistoryDriver;
  // Abstraction to communicate with HistoryService and WebHistoryService.
  std::unique_ptr<history::BrowsingHistoryService> _browsingHistoryService;
}
// Closes history.
- (void)closeHistory;
// Closes history, invoking completionHandler once dismissal is complete.
- (void)closeHistoryWithCompletion:(ProceduralBlock)completionHandler;
// Opens Privacy settings.
- (void)openPrivacySettings;
// Configure view for editing mode.
- (void)enterEditingMode;
// Configure view for non-editing mode.
- (void)exitEditingMode;
// Deletes selected history entries.
- (void)deleteSelectedItems;
// Displays a search bar for searching history entries.
- (void)enterSearchMode;
// Dismisses the search bar.
- (void)exitSearchMode;
// Configures navigation bar for current state of the history collection.
- (void)configureNavigationBar;
// Configures the clear browsing data bar for the current state of the history
// collection.
- (void)configureClearBrowsingBar;

// The dispatcher used by this ViewController.
@property(nonatomic, readonly, weak) id<ApplicationCommands> dispatcher;

@end

@implementation HistoryPanelViewController

@synthesize dispatcher = _dispatcher;

- (instancetype)initWithLoader:(id<UrlLoader>)loader
                  browserState:(ios::ChromeBrowserState*)browserState
                    dispatcher:(id<ApplicationCommands>)dispatcher {
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _historyCollectionController =
        [[LegacyHistoryCollectionViewController alloc]
            initWithLoader:loader
              browserState:browserState
                  delegate:self];

    _browsingHistoryDriver = std::make_unique<IOSBrowsingHistoryDriver>(
        browserState, _historyCollectionController);

    _browsingHistoryService = std::make_unique<history::BrowsingHistoryService>(
        _browsingHistoryDriver.get(),
        ios::HistoryServiceFactory::GetForBrowserState(
            browserState, ServiceAccessType::EXPLICIT_ACCESS),
        IOSChromeProfileSyncServiceFactory::GetForBrowserState(browserState));

    _historyCollectionController.historyService = _browsingHistoryService.get();

    _dispatcher = dispatcher;

    // Configure modal presentation.
    [self setModalPresentationStyle:UIModalPresentationFormSheet];
    [self setModalTransitionStyle:UIModalTransitionStyleCoverVertical];

    // Add and configure header.
    _appBar = [[MDCAppBar alloc] init];
    [self addChildViewController:_appBar.headerViewController];
  }
  return self;
}

- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  NOTREACHED();
  return nil;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  [self setTitle:l10n_util::GetNSString(IDS_HISTORY_TITLE)];

  _containerView = [[UIView alloc] initWithFrame:self.view.frame];
  [_containerView setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                                      UIViewAutoresizingFlexibleHeight];
  [self.view addSubview:_containerView];

  [[_historyCollectionController view]
      setTranslatesAutoresizingMaskIntoConstraints:NO];
  [_historyCollectionController willMoveToParentViewController:self];
  [_containerView addSubview:[_historyCollectionController view]];
  [self addChildViewController:_historyCollectionController];
  [_historyCollectionController didMoveToParentViewController:self];

  _clearBrowsingBar = [[ClearBrowsingBar alloc] initWithFrame:CGRectZero];
  [_clearBrowsingBar setClearBrowsingDataTarget:self
                                         action:@selector(openPrivacySettings)];
  [_clearBrowsingBar setEditTarget:self action:@selector(enterEditingMode)];
  [_clearBrowsingBar setCancelTarget:self action:@selector(exitEditingMode)];
  [_clearBrowsingBar setDeleteTarget:self
                              action:@selector(deleteSelectedItems)];
  [_clearBrowsingBar setTranslatesAutoresizingMaskIntoConstraints:NO];
  [_containerView addSubview:_clearBrowsingBar];
  [self configureClearBrowsingBar];

  ConfigureAppBarWithCardStyle(_appBar);
  [_appBar headerViewController].headerView.trackingScrollView =
      [_historyCollectionController collectionView];
  [_appBar addSubviewsToParent];
  // Prevent the touch events on appBar from being forwarded to the
  // collectionView.  See https://crbug.com/773580
  [_appBar.headerViewController.headerView
      stopForwardingTouchEventsForView:_appBar.navigationBar];

  // Add navigation bar buttons.
  _leftBarButtonItem =
      [ChromeIcon templateBarButtonItemWithImage:[ChromeIcon searchIcon]
                                          target:self
                                          action:@selector(enterSearchMode)];
  self.navigationItem.leftBarButtonItem = _leftBarButtonItem;
  _rightBarButtonItem = [[UIBarButtonItem alloc]
      initWithTitle:l10n_util::GetNSString(IDS_IOS_NAVIGATION_BAR_DONE_BUTTON)
              style:UIBarButtonItemStylePlain
             target:self
             action:@selector(closeHistory)];
  self.navigationItem.rightBarButtonItem = _rightBarButtonItem;
  [self configureNavigationBar];
}

- (void)updateViewConstraints {
  if (!_addedConstraints) {
    NSDictionary* views = @{
      @"collectionView" : [_historyCollectionController view],
      @"clearBrowsingBar" : _clearBrowsingBar,
    };
    NSArray* constraints = @[
      @"V:|[collectionView][clearBrowsingBar]|", @"H:|[collectionView]|",
      @"H:|[clearBrowsingBar]|"
    ];
    ApplyVisualConstraints(constraints, views);
    _addedConstraints = YES;
  }
  [super updateViewConstraints];
}

- (BOOL)disablesAutomaticKeyboardDismissal {
  return NO;
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)orient {
  [super didRotateFromInterfaceOrientation:orient];
  [_clearBrowsingBar updateHeight];
}

#pragma mark - HistoryCollectionViewControllerDelegate

- (void)historyCollectionViewController:
            (LegacyHistoryCollectionViewController*)collectionViewcontroller
              shouldCloseWithCompletion:(ProceduralBlock)completionHandler {
  [self closeHistoryWithCompletion:completionHandler];
}

- (void)historyCollectionViewController:
            (LegacyHistoryCollectionViewController*)controller
                      didScrollToOffset:(CGPoint)offset {
  // Display a shadow on the header when the collection is scrolled.
  MDCFlexibleHeaderView* headerView = _appBar.headerViewController.headerView;
  headerView.visibleShadowOpacity =
      offset.y > -CGRectGetHeight(headerView.frame) ? kShadowOpacity : 0.0f;
}

- (void)historyCollectionViewControllerDidChangeEntries:
    (LegacyHistoryCollectionViewController*)controller {
  // Reconfigure the navigation and clear browsing bars to reflect currently
  // displayed entries.
  [self configureNavigationBar];
  [self configureClearBrowsingBar];
}

- (void)historyCollectionViewControllerDidChangeEntrySelection:
    (LegacyHistoryCollectionViewController*)controller {
  // Reconfigure the clear browsing bar to reflect current availability of
  // entries for deletion.
  [self configureClearBrowsingBar];
}

#pragma mark - HistorySearchViewControllerDelegate

- (void)historySearchViewController:
            (HistorySearchViewController*)searchViewController
            didRequestSearchForTerm:(NSString*)searchTerm {
  [_historyCollectionController showHistoryMatchingQuery:searchTerm];
}

- (void)historySearchViewControllerDidCancel:
    (HistorySearchViewController*)searchViewController {
  DCHECK([_searchViewController isEqual:searchViewController]);
  [self exitSearchMode];
}

#pragma mark UIAccessibilityAction

- (BOOL)accessibilityPerformEscape {
  [self closeHistory];
  return YES;
}

#pragma mark - Private methods

- (void)closeHistory {
  [self closeHistoryWithCompletion:nil];
}

- (void)closeHistoryWithCompletion:(ProceduralBlock)completion {
  [self.presentingViewController dismissViewControllerAnimated:YES
                                                    completion:completion];
}

- (void)openPrivacySettings {
  // Ignore the button tap if view controller presenting.
  if ([self presentedViewController]) {
    return;
  }
  [self exitSearchMode];
  base::RecordAction(
      base::UserMetricsAction("HistoryPage_InitClearBrowsingData"));
  [self.dispatcher showClearBrowsingDataSettingsFromViewController:self];
}

- (void)enterEditingMode {
  // Ignore the button tap if view controller presenting.
  if ([self presentedViewController]) {
    return;
  }
  [_historyCollectionController setEditing:YES];
  [_clearBrowsingBar setEditing:YES];
  if (_historyCollectionController.searching) {
    [_searchViewController setEnabled:NO];
  }
  DCHECK([_historyCollectionController isEditing]);
  [self configureNavigationBar];
}

- (void)exitEditingMode {
  [_historyCollectionController setEditing:NO];
  [_clearBrowsingBar setEditing:NO];
  if (_historyCollectionController.searching) {
    [_searchViewController setEnabled:YES];
  }
  DCHECK(![_historyCollectionController isEditing]);
  [self configureNavigationBar];
}

- (void)deleteSelectedItems {
  [_historyCollectionController deleteSelectedItemsFromHistory];
  base::RecordAction(base::UserMetricsAction("HistoryPage_RemoveSelected"));
  [self exitEditingMode];
}
- (void)enterSearchMode {
  if (!_searchViewController) {
    _searchViewController = [[HistorySearchViewController alloc] init];
    [_searchViewController setDelegate:self];
  }

  UIView* searchBarView = [_searchViewController view];
  [_searchViewController willMoveToParentViewController:self];
  [self.view addSubview:searchBarView];
  _historyCollectionController.searching = YES;
  [_searchViewController didMoveToParentViewController:self];
  base::RecordAction(base::UserMetricsAction("HistoryPage_Search"));

  // Constraints to make search bar cover header.
  [searchBarView setTranslatesAutoresizingMaskIntoConstraints:NO];
  MDCFlexibleHeaderView* headerView = _appBar.headerViewController.headerView;
  NSArray* constraints = @[
    [[searchBarView topAnchor] constraintEqualToAnchor:headerView.topAnchor],
    [[searchBarView leadingAnchor]
        constraintEqualToAnchor:headerView.leadingAnchor],
    [[searchBarView heightAnchor]
        constraintEqualToAnchor:headerView.heightAnchor],
    [[searchBarView widthAnchor] constraintEqualToAnchor:headerView.widthAnchor]
  ];
  [NSLayoutConstraint activateConstraints:constraints];
  // Workaround so navigationItems are not voice-over selectable while hidden by
  // the search view. We might have to re factor the view hierarchy in order to
  // properly solve this issue. See: https://codereview.chromium.org/2605023002/
  self.navigationItem.leftBarButtonItem = nil;
  self.navigationItem.rightBarButtonItem = nil;
}

- (void)exitSearchMode {
  if (_historyCollectionController.searching) {
    // Resets the navigation items to their initial state.
    self.navigationItem.leftBarButtonItem = _leftBarButtonItem;
    self.navigationItem.rightBarButtonItem = _rightBarButtonItem;
    [self configureNavigationBar];

    [[_searchViewController view] removeFromSuperview];
    [_searchViewController removeFromParentViewController];
    _historyCollectionController.searching = NO;
    [_historyCollectionController showHistoryMatchingQuery:nil];
  }
}

- (void)configureNavigationBar {
  // The search button should only be enabled if there are history entries to
  // search, and if history is not in edit mode.
  self.navigationItem.leftBarButtonItem.enabled =
      ![_historyCollectionController isEmpty] &&
      ![_historyCollectionController isEditing];
}

- (void)configureClearBrowsingBar {
  _clearBrowsingBar.editing = _historyCollectionController.editing;
  _clearBrowsingBar.deleteButtonEnabled =
      [_historyCollectionController hasSelectedEntries];
  _clearBrowsingBar.editButtonEnabled = ![_historyCollectionController isEmpty];
}

#pragma mark - UIResponder

- (BOOL)canBecomeFirstResponder {
  return YES;
}

- (NSArray*)keyCommands {
  __weak HistoryPanelViewController* weakSelf = self;
  return @[ [UIKeyCommand cr_keyCommandWithInput:UIKeyInputEscape
                                   modifierFlags:Cr_UIKeyModifierNone
                                           title:nil
                                          action:^{
                                            [weakSelf closeHistory];
                                          }] ];
}

@end
