// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/signin/web_view_gaia_cookie_manager_service_factory.h"

#include "base/memory/singleton.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/signin/core/browser/gaia_cookie_manager_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "google_apis/gaia/gaia_constants.h"
#include "ios/web_view/internal/signin/ios_web_view_signin_client.h"
#include "ios/web_view/internal/signin/web_view_oauth2_token_service_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_client_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

WebViewGaiaCookieManagerServiceFactory::WebViewGaiaCookieManagerServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "GaiaCookieManagerService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(WebViewSigninClientFactory::GetInstance());
  DependsOn(WebViewOAuth2TokenServiceFactory::GetInstance());
}

// static
GaiaCookieManagerService*
WebViewGaiaCookieManagerServiceFactory::GetForBrowserState(
    ios_web_view::WebViewBrowserState* browser_state) {
  return static_cast<GaiaCookieManagerService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
WebViewGaiaCookieManagerServiceFactory*
WebViewGaiaCookieManagerServiceFactory::GetInstance() {
  return base::Singleton<WebViewGaiaCookieManagerServiceFactory>::get();
}

std::unique_ptr<KeyedService>
WebViewGaiaCookieManagerServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  WebViewBrowserState* browser_state =
      WebViewBrowserState::FromBrowserState(context);
  return std::make_unique<GaiaCookieManagerService>(
      WebViewOAuth2TokenServiceFactory::GetForBrowserState(browser_state),
      GaiaConstants::kChromeSource,
      WebViewSigninClientFactory::GetForBrowserState(browser_state));
}

}  // namespace ios_web_view
