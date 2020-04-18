// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/location_bar/location_bar_mediator.h"

#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/security_state/core/security_state_ui.h"
#include "components/toolbar/toolbar_model.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/chrome/browser/ssl/ios_security_state_tab_helper.h"
#import "ios/chrome/browser/ui/location_bar/location_bar_consumer.h"
#include "ios/chrome/browser/ui/omnibox/omnibox_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_list_observer_bridge.h"
#include "ios/chrome/grit/ios_theme_resources.h"
#include "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/ssl_status.h"
#import "ios/web/public/web_client.h"
#include "ios/web/public/web_state/web_state.h"
#import "ios/web/public/web_state/web_state_observer_bridge.h"
#include "skia/ext/skia_utils_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface LocationBarMediator ()<CRWWebStateObserver, WebStateListObserving>
// The current web state associated with the toolbar.
@property(nonatomic, assign) web::WebState* webState;
@end

@implementation LocationBarMediator {
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserver;
  std::unique_ptr<WebStateListObserverBridge> _webStateListObserver;
}

@synthesize consumer = _consumer;
@synthesize webState = _webState;
@synthesize webStateList = _webStateList;
@synthesize toolbarModel = _toolbarModel;

- (instancetype)initWithToolbarModel:(ToolbarModel*)toolbarModel {
  DCHECK(toolbarModel);
  self = [super init];
  if (self) {
    _toolbarModel = toolbarModel;
    _webStateObserver = std::make_unique<web::WebStateObserverBridge>(self);
    _webStateListObserver = std::make_unique<WebStateListObserverBridge>(self);
  }
  return self;
}

- (void)dealloc {
  [self disconnect];
}

#pragma mark - Public

- (void)disconnect {
  if (_webStateList) {
    _webStateList->RemoveObserver(_webStateListObserver.get());
    _webStateListObserver.reset();
    _webStateList = nullptr;
  }

  if (_webState) {
    _webState->RemoveObserver(_webStateObserver.get());
    _webStateObserver.reset();
    _webState = nullptr;
  }
}

#pragma mark - CRWWebStateObserver

- (void)webState:(web::WebState*)webState didLoadPageWithSuccess:(BOOL)success {
  DCHECK_EQ(_webState, webState);
  [self.consumer updateLocationText:[self currentLocationString]];
}

- (void)webState:(web::WebState*)webState
    didStartNavigation:(web::NavigationContext*)navigation {
  DCHECK_EQ(_webState, webState);
  [self.consumer updateLocationText:[self currentLocationString]];
  [self notifyConsumerOfChangedSecurityIcon];
}

- (void)webState:(web::WebState*)webState
    didFinishNavigation:(web::NavigationContext*)navigation {
  DCHECK_EQ(_webState, webState);
  [self.consumer updateLocationText:[self currentLocationString]];
  [self notifyConsumerOfChangedSecurityIcon];
}

- (void)webState:(web::WebState*)webState
    didPruneNavigationItemsWithCount:(size_t)pruned_item_count {
  DCHECK_EQ(_webState, webState);
}

- (void)webStateDidStartLoading:(web::WebState*)webState {
  DCHECK_EQ(_webState, webState);
  [self.consumer updateLocationText:[self currentLocationString]];
  [self notifyConsumerOfChangedSecurityIcon];
}

- (void)webStateDidStopLoading:(web::WebState*)webState {
  DCHECK_EQ(_webState, webState);
  [self.consumer updateLocationText:[self currentLocationString]];
  [self notifyConsumerOfChangedSecurityIcon];
}

- (void)webStateDidChangeVisibleSecurityState:(web::WebState*)webState {
  DCHECK_EQ(_webState, webState);
  [self notifyConsumerOfChangedSecurityIcon];
}

- (void)webStateDestroyed:(web::WebState*)webState {
  DCHECK_EQ(_webState, webState);
  _webState->RemoveObserver(_webStateObserver.get());
  _webState = nullptr;
}

#pragma mark - WebStateListObserver

- (void)webStateList:(WebStateList*)webStateList
    didChangeActiveWebState:(web::WebState*)newWebState
                oldWebState:(web::WebState*)oldWebState
                    atIndex:(int)atIndex
                     reason:(int)reason {
  DCHECK_EQ(_webStateList, webStateList);
  self.webState = newWebState;
  [self.consumer defocusOmnibox];
}

#pragma mark - Setters

- (void)setWebState:(web::WebState*)webState {
  if (_webState) {
    _webState->RemoveObserver(_webStateObserver.get());
  }

  _webState = webState;

  if (_webState) {
    _webState->AddObserver(_webStateObserver.get());

    if (self.consumer) {
      [self.consumer updateLocationText:[self currentLocationString]];
      [self notifyConsumerOfChangedSecurityIcon];
    }
  }
}

- (void)setConsumer:(id<LocationBarConsumer>)consumer {
  _consumer = consumer;
  if (self.webState) {
    [self.consumer updateLocationText:[self currentLocationString]];
    [self notifyConsumerOfChangedSecurityIcon];
  }
}

- (void)setWebStateList:(WebStateList*)webStateList {
  if (_webStateList) {
    _webStateList->RemoveObserver(_webStateListObserver.get());
  }

  _webStateList = webStateList;
  self.webState = self.webStateList->GetActiveWebState();
  _webStateList->AddObserver(_webStateListObserver.get());
}

#pragma mark - private

- (void)notifyConsumerOfChangedSecurityIcon {
  [self.consumer updateLocationIcon:[self currentLocationIcon]];
}

#pragma mark Location helpers

- (NSString*)currentLocationString {
  base::string16 string = self.toolbarModel->GetURLForDisplay();
  return base::SysUTF16ToNSString(string);
}

#pragma mark Security status icon helpers

- (UIImage*)currentLocationIcon {
  return [self imageForSecurityLevel:self.toolbarModel->GetSecurityLevel(true)];
}

- (UIImage*)imageForSecurityLevel:(security_state::SecurityLevel)level {
  int iconID = GetIconForSecurityState(level);
  return [NativeImage(iconID)
      imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
}

@end
