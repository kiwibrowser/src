// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/signin/web_view_signin_client_factory.h"

#include "base/memory/singleton.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/signin/core/browser/signin_client.h"
#include "ios/web_view/internal/content_settings/web_view_cookie_settings_factory.h"
#include "ios/web_view/internal/content_settings/web_view_host_content_settings_map_factory.h"
#include "ios/web_view/internal/signin/ios_web_view_signin_client.h"
#include "ios/web_view/internal/signin/web_view_signin_error_controller_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

// static
IOSWebViewSigninClient* WebViewSigninClientFactory::GetForBrowserState(
    ios_web_view::WebViewBrowserState* browser_state) {
  return static_cast<IOSWebViewSigninClient*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
WebViewSigninClientFactory* WebViewSigninClientFactory::GetInstance() {
  return base::Singleton<WebViewSigninClientFactory>::get();
}

WebViewSigninClientFactory::WebViewSigninClientFactory()
    : BrowserStateKeyedServiceFactory(
          "SigninClient",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(WebViewSigninErrorControllerFactory::GetInstance());
  DependsOn(WebViewCookieSettingsFactory::GetInstance());
  DependsOn(WebViewHostContentSettingsMapFactory::GetInstance());
}

std::unique_ptr<KeyedService>
WebViewSigninClientFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  WebViewBrowserState* browser_state =
      WebViewBrowserState::FromBrowserState(context);
  return std::make_unique<IOSWebViewSigninClient>(
      browser_state->GetPrefs(), browser_state->GetRequestContext(),
      WebViewSigninErrorControllerFactory::GetForBrowserState(browser_state),
      WebViewCookieSettingsFactory::GetForBrowserState(browser_state),
      WebViewHostContentSettingsMapFactory::GetForBrowserState(browser_state),
      nullptr /** scoped_refptr<TokenWebData> */);
}

}  // namespace ios_web_view
