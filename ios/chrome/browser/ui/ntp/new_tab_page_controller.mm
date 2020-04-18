// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/new_tab_page_controller.h"

#import <QuartzCore/QuartzCore.h>

#include "base/logging.h"

#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/sync_sessions/synced_session.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/experimental_flags.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/chrome/browser/search_engines/template_url_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_collection_utils.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_coordinator.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_header_view_controller.h"
#import "ios/chrome/browser/ui/content_suggestions/ntp_home_constant.h"
#import "ios/chrome/browser/ui/ntp/incognito_view_controller.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_bar_item.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_header_constants.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_view.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/web/web_state/ui/crw_swipe_recognizer_provider.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using base::UserMetricsAction;

@interface NewTabPageController () {
  ios::ChromeBrowserState* _browserState;  // weak.
  __weak id<UrlLoader> _loader;
  IncognitoViewController* _incognitoController;
  // The currently visible controller, one of the above.
  __weak id<NewTabPagePanelProtocol> _currentController;

  // Delegate to focus and blur the omnibox.
  __weak id<OmniboxFocuser> _focuser;

  // Delegate to fetch the ToolbarModel and current web state from.
  __weak id<NewTabPageControllerDelegate> _toolbarDelegate;

  TabModel* _tabModel;
}

// Load panel on demand.
- (BOOL)loadPanel:(NewTabPageBarItem*)item;

@property(nonatomic, strong) NewTabPageView* view;

// To ease modernizing the NTP only the internal panels are being converted
// to UIViewControllers.  This means all the plumbing between the
// BrowserViewController and the internal NTP panels (WebController, NTP)
// hierarchy is skipped.  While normally the logic to push and pop a view
// controller would be owned by a coordinator, in this case the old NTP
// controller adds and removes child view controllers itself when a load
// is initiated, and when WebController calls -willBeDismissed.
@property(nonatomic, weak) UIViewController* parentViewController;

// The command dispatcher.
@property(nonatomic, weak) id<ApplicationCommands,
                              BrowserCommands,
                              OmniboxFocuser,
                              FakeboxFocuser,
                              SnackbarCommands,
                              UrlLoader>
    dispatcher;

// Panel displaying the "Home" view, with the logo and the fake omnibox.
@property(nonatomic, strong) id<NewTabPagePanelProtocol> homePanel;

// Coordinator for the ContentSuggestions.
@property(nonatomic, strong)
    ContentSuggestionsCoordinator* contentSuggestionsCoordinator;

// Controller for the header of the Home panel.
@property(nonatomic, strong) id<LogoAnimationControllerOwnerOwner, ToolbarOwner>
    headerController;

@end

@implementation NewTabPageController

@synthesize view = _view;
@synthesize swipeRecognizerProvider = _swipeRecognizerProvider;
@synthesize parentViewController = _parentViewController;
@synthesize dispatcher = _dispatcher;
@synthesize homePanel = _homePanel;
@synthesize contentSuggestionsCoordinator = _contentSuggestionsCoordinator;
@synthesize headerController = _headerController;

- (id)initWithUrl:(const GURL&)url
                  loader:(id<UrlLoader>)loader
                 focuser:(id<OmniboxFocuser>)focuser
            browserState:(ios::ChromeBrowserState*)browserState
         toolbarDelegate:(id<NewTabPageControllerDelegate>)toolbarDelegate
                tabModel:(TabModel*)tabModel
    parentViewController:(UIViewController*)parentViewController
              dispatcher:(id<ApplicationCommands,
                             BrowserCommands,
                             OmniboxFocuser,
                             FakeboxFocuser,
                             SnackbarCommands,
                             UrlLoader>)dispatcher
           safeAreaInset:(UIEdgeInsets)safeAreaInset {
  self = [super initWithNibName:nil url:url];
  if (self) {
    DCHECK(browserState);
    _browserState = browserState;
    _loader = loader;
    _parentViewController = parentViewController;
    _dispatcher = dispatcher;
    _focuser = focuser;
    _toolbarDelegate = toolbarDelegate;
    _tabModel = tabModel;
    self.title = l10n_util::GetNSString(IDS_NEW_TAB_TITLE);

    NewTabPageBar* tabBar =
        [[NewTabPageBar alloc] initWithFrame:CGRectMake(0, 412, 320, 48)];
    _view = [[NewTabPageView alloc] initWithFrame:CGRectMake(0, 0, 320, 460)
                                        andTabBar:tabBar];
    _view.safeAreaInsetForToolbar = safeAreaInset;
    [tabBar setDelegate:self];

    bool isIncognito = _browserState->IsOffTheRecord();

    NSString* incognito = l10n_util::GetNSString(IDS_IOS_NEW_TAB_INCOGNITO);
    NSString* home = l10n_util::GetNSString(IDS_IOS_NEW_TAB_HOME);
    NSString* bookmarks =
        l10n_util::GetNSString(IDS_IOS_NEW_TAB_BOOKMARKS_PAGE_TITLE_MOBILE);
    NSString* openTabs = l10n_util::GetNSString(IDS_IOS_NEW_TAB_RECENT_TABS);

    NSMutableArray* tabBarItems = [NSMutableArray array];
    NewTabPageBarItem* itemToDisplay = nil;
    if (isIncognito) {
      NewTabPageBarItem* incognitoItem = [NewTabPageBarItem
          newTabPageBarItemWithTitle:incognito
                          identifier:ntp_home::INCOGNITO_PANEL
                               image:[UIImage imageNamed:@"ntp_incognito"]];
      itemToDisplay = incognitoItem;
    } else {
      NewTabPageBarItem* homeItem = [NewTabPageBarItem
          newTabPageBarItemWithTitle:home
                          identifier:ntp_home::HOME_PANEL
                               image:[UIImage imageNamed:@"ntp_mv_search"]];
      NewTabPageBarItem* bookmarksItem = [NewTabPageBarItem
          newTabPageBarItemWithTitle:bookmarks
                          identifier:ntp_home::BOOKMARKS_PANEL
                               image:[UIImage imageNamed:@"ntp_bookmarks"]];
      [tabBarItems addObject:bookmarksItem];
      NewTabPageBarItem* openTabsItem = [NewTabPageBarItem
          newTabPageBarItemWithTitle:openTabs
                          identifier:ntp_home::RECENT_TABS_PANEL
                               image:[UIImage imageNamed:@"ntp_opentabs"]];
      [tabBarItems addObject:openTabsItem];
      self.view.tabBar.items = tabBarItems;
      itemToDisplay = homeItem;
      base::RecordAction(UserMetricsAction("MobileNTPShowMostVisited"));
    }
    DCHECK(itemToDisplay);
    [self loadPanel:itemToDisplay];
    if (isIncognito) {
      _currentController = self.incognitoController;
    } else {
      _currentController = self.homePanel;
    }
    [_currentController wasShown];
  }
  return self;
}

- (void)dealloc {
  // This is not an ideal place to put view controller contaimnent, rather a
  // //web -wasDismissed method on CRWNativeContent would be more accurate. If
  // CRWNativeContent leaks, this will not be called.
  [_incognitoController removeFromParentViewController];
  [[self.contentSuggestionsCoordinator viewController]
      removeFromParentViewController];

  [self.contentSuggestionsCoordinator stop];

  [self.homePanel setDelegate:nil];
}

#pragma mark - Properties

- (UIEdgeInsets)contentInset {
  return self.contentSuggestionsCoordinator.viewController.collectionView
      .contentInset;
}

- (void)setContentInset:(UIEdgeInsets)contentInset {
  // UIKit will adjust the contentOffset sometimes when changing the
  // contentInset.bottom.  We don't want the NTP to scroll, so store and re-set
  // the contentOffset after setting the contentInset.
  CGPoint contentOffset = self.contentSuggestionsCoordinator.viewController
                              .collectionView.contentOffset;
  self.contentSuggestionsCoordinator.viewController.collectionView
      .contentInset = contentInset;
  self.contentSuggestionsCoordinator.viewController.collectionView
      .contentOffset = contentOffset;
}

#pragma mark - CRWNativeContent

- (void)willBeDismissed {
  // This methods is called by //web immediately before |self|'s view is removed
  // from the view hierarchy, making it an ideal spot to intiate view controller
  // containment methods.
  [[self.contentSuggestionsCoordinator viewController]
      willMoveToParentViewController:nil];
  [_incognitoController willMoveToParentViewController:nil];
}

- (void)reload {
  [_currentController reload];
  [super reload];
}

- (void)wasShown {
  [_currentController wasShown];
  if (_currentController != self.homePanel) {
    // Ensure that the NTP has the latest data when it is shown, except for
    // Home.
    [self reload];
  }
  [self.view.tabBar setShadowAlpha:[_currentController alphaForBottomShadow]];
}

- (void)wasHidden {
  [_currentController wasHidden];
}

- (BOOL)wantsLocationBarHintText {
  // Always show hint text on iPhone.
  if (!IsIPadIdiom())
    return YES;
  // Always show the location bar hint text if the search engine is not Google.
  TemplateURLService* service =
      ios::TemplateURLServiceFactory::GetForBrowserState(_browserState);
  if (service) {
    const TemplateURL* defaultURL = service->GetDefaultSearchProvider();
    if (defaultURL &&
        defaultURL->GetEngineType(service->search_terms_data()) !=
            SEARCH_ENGINE_GOOGLE) {
      return YES;
    }
  }

  // Always return true when incognito.
  if (_browserState->IsOffTheRecord())
    return YES;

  return NO;
}

- (void)dismissModals {
  [_currentController dismissModals];
}

- (void)willUpdateSnapshot {
  [_currentController willUpdateSnapshot];
}

- (CGPoint)scrollOffset {
  return [_currentController scrollOffset];
}

#pragma mark -

// Called when the user presses a segment that's not currently selected.
// Pressing a segment that's already selected does not trigger this action.
- (void)newTabBarItemDidChange:(NewTabPageBarItem*)selectedItem {
  if (selectedItem.identifier == ntp_home::BOOKMARKS_PANEL) {
    [self.dispatcher showBookmarksManager];
  } else if (selectedItem.identifier == ntp_home::RECENT_TABS_PANEL) {
    [self.dispatcher showRecentTabs];
  }

  if (_browserState->IsOffTheRecord())
    return;

  // Update metrics. Intentionally omitting a metric for Incognito panel.
  if (selectedItem.identifier == ntp_home::HOME_PANEL) {
    base::RecordAction(UserMetricsAction("MobileNTPSwitchToMostVisited"));
  } else if (selectedItem.identifier == ntp_home::RECENT_TABS_PANEL) {
    base::RecordAction(UserMetricsAction("MobileNTPSwitchToOpenTabs"));
  } else if (selectedItem.identifier == ntp_home::BOOKMARKS_PANEL) {
    base::RecordAction(UserMetricsAction("MobileNTPSwitchToBookmarks"));
  }
}

- (BOOL)loadPanel:(NewTabPageBarItem*)item {
  DCHECK(self.parentViewController);
  UIViewController* panelController = nil;
  UICollectionView* collectionView = nil;
  // Only load the controllers once.
  if (item.identifier == ntp_home::HOME_PANEL) {
    if (!self.contentSuggestionsCoordinator) {
      self.contentSuggestionsCoordinator = [
          [ContentSuggestionsCoordinator alloc] initWithBaseViewController:nil];
      self.contentSuggestionsCoordinator.URLLoader = _loader;
      self.contentSuggestionsCoordinator.browserState = _browserState;
      self.contentSuggestionsCoordinator.dispatcher = self.dispatcher;
      self.contentSuggestionsCoordinator.webStateList =
          [_tabModel webStateList];
      self.contentSuggestionsCoordinator.toolbarDelegate = _toolbarDelegate;
      [self.contentSuggestionsCoordinator start];
      self.headerController =
          self.contentSuggestionsCoordinator.headerController;
    }
    panelController = [self.contentSuggestionsCoordinator viewController];
    collectionView =
        self.contentSuggestionsCoordinator.viewController.collectionView;
    self.homePanel = self.contentSuggestionsCoordinator;
    [self.homePanel setDelegate:self];
  } else if (item.identifier == ntp_home::INCOGNITO_PANEL) {
    if (!_incognitoController)
      _incognitoController =
          [[IncognitoViewController alloc] initWithLoader:_loader
                                          toolbarDelegate:_toolbarDelegate];
    panelController = _incognitoController;
  } else {
    NOTREACHED();
    return NO;
  }

  UIView* view = panelController.view;
  if (item.identifier == ntp_home::HOME_PANEL) {
    // Update the shadow for the toolbar after the view creation.
    [self.view.tabBar setShadowAlpha:[self.homePanel alphaForBottomShadow]];
  }

  BOOL created = NO;
  if (view.superview == nil) {
    created = YES;
    item.view = view;

    // To ease modernizing the NTP only the internal panels are being converted
    // to UIViewControllers.  This means all the plumbing between the
    // BrowserViewController and the internal NTP panels (WebController, NTP)
    // hierarchy is skipped.  While normally the logic to push and pop a view
    // controller would be owned by a coordinator, in this case the old NTP
    // controller adds and removes child view controllers itself when a load
    // is initiated, and when WebController calls -willBeDismissed.
    DCHECK(panelController);
    [self.parentViewController addChildViewController:panelController];
    [self.view insertSubview:view belowSubview:self.view.tabBar];
    self.view.contentView = view;
    self.view.contentCollectionView = collectionView;
    [panelController didMoveToParentViewController:self.parentViewController];
  }
  return created;
}


#pragma mark - LogoAnimationControllerOwnerOwner

- (id<LogoAnimationControllerOwner>)logoAnimationControllerOwner {
  return [self.headerController logoAnimationControllerOwner];
}

#pragma mark -
#pragma mark ToolbarOwner

- (CGRect)toolbarFrame {
  return [self.headerController toolbarFrame];
}

- (id<ToolbarSnapshotProviding>)toolbarSnapshotProvider {
  return self.headerController.toolbarSnapshotProvider;
}

- (CGFloat)toolbarHeight {
  BOOL isRegularXRegular =
      content_suggestions::IsRegularXRegularSizeClass(self.view);
  // If the google landing controller is nil, there is no toolbar visible in the
  // native content view, finally there is no toolbar on iPad.
  return self.headerController && !isRegularXRegular
             ? ntp_header::ToolbarHeight()
             : 0.0;
}

#pragma mark - NewTabPagePanelControllerDelegate

- (void)updateNtpBarShadowForPanelController:
    (id<NewTabPagePanelProtocol>)ntpPanelController {
  if (_currentController != ntpPanelController)
    return;
  [self.view.tabBar setShadowAlpha:[ntpPanelController alphaForBottomShadow]];
}

@end

@implementation NewTabPageController (TestSupport)

- (id<NewTabPagePanelProtocol>)currentController {
  return _currentController;
}

- (id<NewTabPagePanelProtocol>)incognitoController {
  return _incognitoController;
}

@end
