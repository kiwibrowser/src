// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/cwv_web_view_configuration_internal.h"

#include <memory>

#include "base/logging.h"
#include "base/threading/thread_restrictions.h"
#include "ios/web_view/cwv_web_view_features.h"
#include "ios/web_view/internal/app/application_context.h"
#import "ios/web_view/internal/autofill/cwv_autofill_data_manager_internal.h"
#include "ios/web_view/internal/autofill/web_view_personal_data_manager_factory.h"
#import "ios/web_view/internal/cwv_preferences_internal.h"
#import "ios/web_view/internal/cwv_user_content_controller_internal.h"
#import "ios/web_view/internal/cwv_web_view_internal.h"
#import "ios/web_view/internal/signin/cwv_authentication_controller_internal.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "ios/web_view/internal/web_view_global_state_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CWVWebViewConfiguration () {
  // The BrowserState for this configuration.
  std::unique_ptr<ios_web_view::WebViewBrowserState> _browserState;

  // Holds all CWVWebViews created with this class. Weak references.
  NSHashTable* _webViews;

  // |YES| if |shutDown| was called.
  BOOL _wasShutDown;
}

#if BUILDFLAG(IOS_WEB_VIEW_ENABLE_SIGNIN)
// This web view configuration's authentication controller.
// nil if CWVWebViewConfiguration is created with +incognitoConfiguration.
@property(nonatomic, readonly, nullable)
    CWVAuthenticationController* authenticationController;
#endif  // BUILDFLAG(IOS_WEB_VIEW_ENABLE_SIGNIN)

#if BUILDFLAG(IOS_WEB_VIEW_ENABLE_AUTOFILL)
// This web view configuration's autofill data manager.
// nil if CWVWebViewConfiguration is created with +incognitoConfiguration.
@property(nonatomic, readonly, nullable)
    CWVAutofillDataManager* autofillDataManager;
#endif  // BUILDFLAG(IOS_WEB_VIEW_ENABLE_AUTOFILL)

// Initializes configuration with the specified browser state mode.
- (instancetype)initWithBrowserState:
    (std::unique_ptr<ios_web_view::WebViewBrowserState>)browserState;

@end

@implementation CWVWebViewConfiguration

#if BUILDFLAG(IOS_WEB_VIEW_ENABLE_SIGNIN)
@synthesize authenticationController = _authenticationController;
#endif  // BUILDFLAG(IOS_WEB_VIEW_ENABLE_SIGNIN)
#if BUILDFLAG(IOS_WEB_VIEW_ENABLE_AUTOFILL)
@synthesize autofillDataManager = _autofillDataManager;
#endif  // BUILDFLAG(IOS_WEB_VIEW_ENABLE_AUTOFILL)
@synthesize preferences = _preferences;
@synthesize userContentController = _userContentController;

namespace {
CWVWebViewConfiguration* gDefaultConfiguration = nil;
CWVWebViewConfiguration* gIncognitoConfiguration = nil;
}  // namespace

+ (void)shutDown {
  [gDefaultConfiguration shutDown];
  [gIncognitoConfiguration shutDown];
}

+ (instancetype)defaultConfiguration {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    auto browserState = std::make_unique<ios_web_view::WebViewBrowserState>(
        /*off_the_record = */ false);
    gDefaultConfiguration = [[CWVWebViewConfiguration alloc]
        initWithBrowserState:std::move(browserState)];
  });
  return gDefaultConfiguration;
}

+ (instancetype)incognitoConfiguration {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    CWVWebViewConfiguration* defaultConfiguration = [self defaultConfiguration];
    auto browserState = std::make_unique<ios_web_view::WebViewBrowserState>(
        /*off_the_record = */ true, defaultConfiguration.browserState);
    gIncognitoConfiguration = [[CWVWebViewConfiguration alloc]
        initWithBrowserState:std::move(browserState)];
  });
  return gIncognitoConfiguration;
}

+ (void)initialize {
  if (self != [CWVWebViewConfiguration class]) {
    return;
  }

  ios_web_view::InitializeGlobalState();
}

- (instancetype)initWithBrowserState:
    (std::unique_ptr<ios_web_view::WebViewBrowserState>)browserState {
  self = [super init];
  if (self) {
    _browserState = std::move(browserState);

    _preferences =
        [[CWVPreferences alloc] initWithPrefService:_browserState->GetPrefs()];

    _userContentController =
        [[CWVUserContentController alloc] initWithConfiguration:self];

    _webViews = [NSHashTable weakObjectsHashTable];
  }
  return self;
}

- (void)dealloc {
  DCHECK(_wasShutDown);
}

#if BUILDFLAG(IOS_WEB_VIEW_ENABLE_SIGNIN)
#pragma mark - Signin

- (CWVAuthenticationController*)authenticationController {
  if (!_authenticationController && self.persistent) {
    _authenticationController = [[CWVAuthenticationController alloc]
        initWithBrowserState:self.browserState];
  }
  return _authenticationController;
}
#endif  // BUILDFLAG(IOS_WEB_VIEW_ENABLE_SIGNIN)

#if BUILDFLAG(IOS_WEB_VIEW_ENABLE_AUTOFILL)
#pragma mark - Autofill

- (CWVAutofillDataManager*)autofillDataManager {
  if (!_autofillDataManager && self.persistent) {
    autofill::PersonalDataManager* personalDataManager =
        ios_web_view::WebViewPersonalDataManagerFactory::GetForBrowserState(
            self.browserState);
    _autofillDataManager = [[CWVAutofillDataManager alloc]
        initWithPersonalDataManager:personalDataManager];
  }
  return _autofillDataManager;
}

#endif  // BUILDFLAG(IOS_WEB_VIEW_ENABLE_AUTOFILL)

#pragma mark - Public Methods

- (BOOL)isPersistent {
  return !_browserState->IsOffTheRecord();
}

#pragma mark - Private Methods

- (ios_web_view::WebViewBrowserState*)browserState {
  return _browserState.get();
}

- (void)registerWebView:(CWVWebView*)webView {
  [_webViews addObject:webView];
}

- (void)shutDown {
  for (CWVWebView* webView in _webViews) {
    [webView shutDown];
  }
  _browserState.reset();
  _wasShutDown = YES;
}

@end
