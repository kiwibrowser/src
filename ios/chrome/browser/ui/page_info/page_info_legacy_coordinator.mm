// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/page_info/page_info_legacy_coordinator.h"

#import <Foundation/Foundation.h>

#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/chrome/browser/reading_list/offline_url_utils.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/commands/page_info_commands.h"
#import "ios/chrome/browser/ui/fullscreen/chrome_coordinator+fullscreen_disabling.h"
#include "ios/chrome/browser/ui/page_info/page_info_model.h"
#import "ios/chrome/browser/ui/page_info/page_info_view_controller.h"
#import "ios/chrome/browser/ui/page_info/requirements/page_info_presentation.h"
#import "ios/chrome/browser/ui/page_info/requirements/page_info_reloading.h"
#import "ios/chrome/browser/ui/url_loader.h"
#include "ios/web/public/navigation_item.h"
#include "ios/web/public/navigation_manager.h"
#include "ios/web/public/reload_type.h"
#include "ios/web/public/web_client.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* const kPageInfoWillShowNotification =
    @"kPageInfoWillShowNotification";

NSString* const kPageInfoWillHideNotification =
    @"kPageInfoWillHideNotification";

@interface PageInfoLegacyCoordinator ()<PageInfoCommands, PageInfoReloading>

// The view controller for the Page Info UI. Nil if not visible.
@property(nonatomic, strong) PageInfoViewController* pageInfoViewController;

@end

@implementation PageInfoLegacyCoordinator

@synthesize dispatcher = _dispatcher;
@synthesize loader = _loader;
@synthesize pageInfoViewController = _pageInfoViewController;
@synthesize presentationProvider = _presentationProvider;
@synthesize tabModel = _tabModel;

- (void)disconnect {
  // DCHECK that the Page Info UI is not displayed before disconnecting.
  DCHECK(!self.pageInfoViewController);
  [self.dispatcher stopDispatchingToTarget:self];
  self.dispatcher = nil;
  self.loader = nil;
  self.presentationProvider = nil;
  self.tabModel = nil;
}

- (void)setDispatcher:(CommandDispatcher*)dispatcher {
  if (dispatcher == self.dispatcher)
    return;
  if (self.dispatcher)
    [self.dispatcher stopDispatchingToTarget:self];
  [dispatcher startDispatchingToTarget:self
                           forProtocol:@protocol(PageInfoCommands)];
  _dispatcher = dispatcher;
}

#pragma mark - PageInfoCommands

- (void)showPageInfoForOriginPoint:(CGPoint)originPoint {
  Tab* tab = self.tabModel.currentTab;
  DCHECK([tab navigationManager]);
  web::NavigationItem* navItem = [tab navigationManager]->GetVisibleItem();

  // It is fully expected to have a navItem here, as showPageInfoPopup can only
  // be trigerred by a button enabled when a current item matches some
  // conditions. However a crash was seen were navItem was NULL hence this
  // test after a DCHECK.
  DCHECK(navItem);
  if (!navItem)
    return;

  // Don't show if the page is native except for offline pages (to show the
  // offline page info).
  if (web::GetWebClient()->IsAppSpecificURL(navItem->GetURL()) &&
      !reading_list::IsOfflineURL(navItem->GetURL())) {
    return;
  }

  // Don't show the bubble twice (this can happen when tapping very quickly in
  // accessibility mode).
  if (self.pageInfoViewController)
    return;

  base::RecordAction(base::UserMetricsAction("MobileToolbarPageSecurityInfo"));

  [self.presentationProvider prepareForPageInfoPresentation];

  [[NSNotificationCenter defaultCenter]
      postNotificationName:kPageInfoWillShowNotification
                    object:nil];

  // Disable fullscreen while the page info UI is displayed.
  [self didStartFullscreenDisablingUI];

  // TODO(crbug.com/760387): Get rid of PageInfoModel completely.
  PageInfoModelBubbleBridge* bridge = new PageInfoModelBubbleBridge();
  PageInfoModel* pageInfoModel = new PageInfoModel(
      self.browserState, navItem->GetURL(), navItem->GetSSL(), bridge);

  CGPoint originPresentationCoordinates = [self.presentationProvider
      convertToPresentationCoordinatesForOrigin:originPoint];
  self.pageInfoViewController = [[PageInfoViewController alloc]
             initWithModel:pageInfoModel
                    bridge:bridge
               sourcePoint:originPresentationCoordinates
      presentationProvider:self.presentationProvider
                dispatcher:self];
  bridge->set_controller(self.pageInfoViewController);
}

- (void)hidePageInfo {
  // Early return if the PageInfoPopup is not presented.
  if (!self.pageInfoViewController)
    return;

  [[NSNotificationCenter defaultCenter]
      postNotificationName:kPageInfoWillHideNotification
                    object:nil];

  // Stop disabling fullscreen since the page info UI was stopped.
  [self didStopFullscreenDisablingUI];

  [self.pageInfoViewController dismiss];
  self.pageInfoViewController = nil;
}

- (void)showSecurityHelpPage {
  [self.loader webPageOrderedOpen:GURL(kPageInfoHelpCenterURL)
                         referrer:web::Referrer()
                     inBackground:NO
                         appendTo:kCurrentTab];
  [self hidePageInfo];
}

#pragma mark - PageInfoReloading

- (void)reload {
  web::WebState* webState = self.tabModel.currentTab.webState;
  if (webState) {
    // |check_for_repost| is true because the reload is explicitly initiated
    // by the user.
    webState->GetNavigationManager()->Reload(web::ReloadType::NORMAL,
                                             true /* check_for_repost */);
  }
}

@end
