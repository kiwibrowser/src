// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/tools_popup_controller.h"

#import <QuartzCore/QuartzCore.h>

#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#import "ios/chrome/browser/ui/commands/application_commands.h"
#import "ios/chrome/browser/ui/popup_menu/popup_menu_view.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_configuration.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_view_controller.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using base::UserMetricsAction;

namespace {

const CGFloat kToolsPopupMenuWidth = 280.0;
const CGFloat kToolsPopupMenuTrailingOffset = 4;

// Inset for the shadows of the contained views.
NS_INLINE UIEdgeInsets TabHistoryPopupMenuInsets() {
  return UIEdgeInsetsMake(9, 11, 12, 11);
}

}  // namespace

@interface ToolsPopupController ()<ToolsPopupTableDelegate> {
  ToolsMenuViewController* _toolsMenuViewController;
  // Container view of the menu items table.
  UIView* _toolsTableViewContainer;
  // The view controller from which to present other view controllers.
  __weak UIViewController* _baseViewController;
}
@end

@implementation ToolsPopupController
@synthesize isCurrentPageBookmarked = _isCurrentPageBookmarked;

- (instancetype)
initAndPresentWithConfiguration:(ToolsMenuConfiguration*)configuration
                     dispatcher:
                         (id<ApplicationCommands, BrowserCommands>)dispatcher
                     completion:(ProceduralBlock)completion {
  DCHECK(configuration.displayView);
  self = [super initWithParentView:configuration.displayView];
  if (self) {
    // Set superclass dispatcher property.
    self.dispatcher = dispatcher;
    _toolsMenuViewController = [[ToolsMenuViewController alloc] init];
    _toolsMenuViewController.dispatcher = self.dispatcher;

    _baseViewController = configuration.baseViewController;

    _toolsTableViewContainer = [_toolsMenuViewController view];
    [_toolsTableViewContainer layer].cornerRadius = 2;
    [_toolsTableViewContainer layer].masksToBounds = YES;
    [_toolsMenuViewController initializeMenuWithConfiguration:configuration];

    UIEdgeInsets popupInsets = TabHistoryPopupMenuInsets();
    CGFloat popupWidth = kToolsPopupMenuWidth;

    CGPoint origin = CGPointMake(CGRectGetMidX(configuration.sourceRect),
                                 CGRectGetMidY(configuration.sourceRect));

    CGRect containerBounds = [configuration.displayView bounds];
    CGFloat minY = CGRectGetMinY(configuration.sourceRect) - popupInsets.top;

    UIEdgeInsets safeAreaInsets = UIEdgeInsetsZero;
    if (@available(iOS 11.0, *)) {
      safeAreaInsets = configuration.displayView.safeAreaInsets;
    }

    // The tools popup appears trailing- aligned, but because
    // kToolsPopupMenuTrailingOffset is smaller than the popupInsets's trailing
    // value, destination needs to be shifted a bit.
    CGFloat trailingShift =
        UIEdgeInsetsGetTrailing(popupInsets) - kToolsPopupMenuTrailingOffset;
    // The tools popup needs to be displayed inside the safe area.
    trailingShift -= UIEdgeInsetsGetTrailing(safeAreaInsets);

    if (UseRTLLayout())
      trailingShift = -trailingShift;

    CGPoint destination = CGPointMake(
        CGRectGetTrailingEdge(containerBounds) + trailingShift, minY);

    CGFloat availableHeight =
        CGRectGetHeight([configuration.displayView bounds]) - minY -
        popupInsets.bottom;
    CGFloat optimalHeight =
        [_toolsMenuViewController optimalHeight:availableHeight];
    [self setOptimalSize:CGSizeMake(popupWidth, optimalHeight)
                atOrigin:destination];

    CGRect bounds = [[self popupContainer] bounds];
    CGRect frame = UIEdgeInsetsInsetRect(bounds, popupInsets);

    [_toolsTableViewContainer setFrame:frame];
    [[self popupContainer] addSubview:_toolsTableViewContainer];

    [_toolsMenuViewController setDelegate:self];
    [self fadeInPopupFromSource:origin
                  toDestination:destination
                     completion:completion];

    // Insert |toolsButton| above |popupContainer| so it appears stationary.
    // Otherwise the tools button will animate with the tools popup.
    UIButton* toolsButton = [_toolsMenuViewController toolsButton];
    if (toolsButton) {
      UIView* outsideAnimationView = [[self popupContainer] superview];
      const CGFloat buttonWidth = 48;
      // |origin| is the center of the tools menu icon in the toolbar; use
      // that to determine where the tools button should be placed.
      CGPoint buttonCenter =
          [configuration.displayView convertPoint:origin
                                           toView:outsideAnimationView];
      CGRect frame = CGRectMake(buttonCenter.x - buttonWidth / 2.0,
                                buttonCenter.y - buttonWidth / 2.0, buttonWidth,
                                buttonWidth);
      [toolsButton setFrame:frame];
      [toolsButton setImageEdgeInsets:configuration.toolsButtonInsets];
      [outsideAnimationView addSubview:toolsButton];
    }
  }
  return self;
}

- (void)dealloc {
  [_toolsTableViewContainer removeFromSuperview];
  [_toolsMenuViewController setDelegate:nil];
}

- (void)fadeInPopupFromSource:(CGPoint)source
                toDestination:(CGPoint)destination
                   completion:(ProceduralBlock)completion {
  [_toolsMenuViewController animateContentIn];
  [super fadeInPopupFromSource:source
                 toDestination:destination
                    completion:completion];
}

- (void)dismissAnimatedWithCompletion:(void (^)(void))completion {
  [_toolsMenuViewController hideContent];
  [super dismissAnimatedWithCompletion:completion];
}

- (void)setIsCurrentPageBookmarked:(BOOL)value {
  _isCurrentPageBookmarked = value;
  [_toolsMenuViewController setIsCurrentPageBookmarked:value];
}

- (void)setCanShowFindBar:(BOOL)enabled {
  [_toolsMenuViewController setCanShowFindBar:enabled];
}

- (void)setCanShowShareMenu:(BOOL)enabled {
  [_toolsMenuViewController setCanShowShareMenu:enabled];
}

- (void)setIsTabLoading:(BOOL)isTabLoading {
  [_toolsMenuViewController setIsTabLoading:isTabLoading];
}

#pragma mark - ToolsPopupTableDelegate methods

- (void)commandWasSelected:(int)commandID {
  // Record the corresponding metric.
  switch (commandID) {
    case TOOLS_BOOKMARK_EDIT:
      base::RecordAction(UserMetricsAction("MobileMenuEditBookmark"));
      break;
    case TOOLS_BOOKMARK_ITEM:
      base::RecordAction(UserMetricsAction("MobileMenuAddToBookmarks"));
      break;
    case TOOLS_CLOSE_ALL_TABS:
      base::RecordAction(UserMetricsAction("MobileMenuCloseAllTabs"));
      break;
    case TOOLS_CLOSE_ALL_INCOGNITO_TABS:
      base::RecordAction(UserMetricsAction("MobileMenuCloseAllIncognitoTabs"));
      break;
    case TOOLS_SHOW_FIND_IN_PAGE:
      base::RecordAction(UserMetricsAction("MobileMenuFindInPage"));
      break;
    case TOOLS_SHOW_HELP_PAGE:
      base::RecordAction(UserMetricsAction("MobileMenuHelp"));
      break;
    case TOOLS_NEW_INCOGNITO_TAB_ITEM:
      base::RecordAction(UserMetricsAction("MobileMenuNewIncognitoTab"));
      break;
    case TOOLS_NEW_TAB_ITEM:
      base::RecordAction(UserMetricsAction("MobileMenuNewTab"));
      break;
    case TOOLS_SETTINGS_ITEM:
      base::RecordAction(UserMetricsAction("MobileMenuSettings"));
      [self.dispatcher showSettingsFromViewController:_baseViewController];
      break;
    case TOOLS_RELOAD_ITEM:
      base::RecordAction(UserMetricsAction("MobileMenuReload"));
      break;
    case TOOLS_SHARE_ITEM:
      base::RecordAction(UserMetricsAction("MobileMenuShare"));
      break;
    case TOOLS_REQUEST_DESKTOP_SITE:
      base::RecordAction(UserMetricsAction("MobileMenuRequestDesktopSite"));
      break;
    case TOOLS_REQUEST_MOBILE_SITE:
      base::RecordAction(UserMetricsAction("MobileMenuRequestMobileSite"));
      break;
    case TOOLS_SHOW_BOOKMARKS:
      base::RecordAction(UserMetricsAction("MobileMenuAllBookmarks"));
      break;
    case TOOLS_SHOW_HISTORY:
      base::RecordAction(UserMetricsAction("MobileMenuHistory"));
      break;
    case TOOLS_SHOW_RECENT_TABS:
      base::RecordAction(UserMetricsAction("MobileMenuRecentTabs"));
      break;
    case TOOLS_STOP_ITEM:
      base::RecordAction(UserMetricsAction("MobileMenuStop"));
      break;
    case TOOLS_REPORT_AN_ISSUE:
      self.containerView.hidden = YES;
      base::RecordAction(UserMetricsAction("MobileMenuReportAnIssue"));
      [self.dispatcher showReportAnIssueFromViewController:_baseViewController];
      break;
    case TOOLS_VIEW_SOURCE:
      // Debug only; no metric.
      break;
    case TOOLS_MENU_ITEM:
      // Do nothing when tapping the tools menu a second time.
      break;
    case TOOLS_READING_LIST:
      base::RecordAction(UserMetricsAction("MobileMenuReadingList"));
      break;
    default:
      NOTREACHED();
      break;
  }

  // Close the menu.
  [self.delegate dismissPopupMenu:self];
}

@end
