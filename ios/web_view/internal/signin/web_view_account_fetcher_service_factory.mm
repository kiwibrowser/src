// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/signin/web_view_account_fetcher_service_factory.h"

#include <utility>

#include "base/memory/singleton.h"
#include "components/image_fetcher/ios/ios_image_decoder_impl.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/signin/core/browser/account_fetcher_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "ios/web_view/internal/signin/ios_web_view_signin_client.h"
#include "ios/web_view/internal/signin/web_view_account_tracker_service_factory.h"
#include "ios/web_view/internal/signin/web_view_oauth2_token_service_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_client_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

WebViewAccountFetcherServiceFactory::WebViewAccountFetcherServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "AccountFetcherService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(WebViewAccountTrackerServiceFactory::GetInstance());
  DependsOn(WebViewOAuth2TokenServiceFactory::GetInstance());
  DependsOn(WebViewSigninClientFactory::GetInstance());
}

// static
AccountFetcherService* WebViewAccountFetcherServiceFactory::GetForBrowserState(
    ios_web_view::WebViewBrowserState* browser_state) {
  return static_cast<AccountFetcherService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
WebViewAccountFetcherServiceFactory*
WebViewAccountFetcherServiceFactory::GetInstance() {
  return base::Singleton<WebViewAccountFetcherServiceFactory>::get();
}

void WebViewAccountFetcherServiceFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  AccountFetcherService::RegisterPrefs(registry);
}

std::unique_ptr<KeyedService>
WebViewAccountFetcherServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  WebViewBrowserState* browser_state =
      WebViewBrowserState::FromBrowserState(context);
  std::unique_ptr<AccountFetcherService> service =
      std::make_unique<AccountFetcherService>();
  service->Initialize(
      WebViewSigninClientFactory::GetForBrowserState(browser_state),
      WebViewOAuth2TokenServiceFactory::GetForBrowserState(browser_state),
      WebViewAccountTrackerServiceFactory::GetForBrowserState(browser_state),
      image_fetcher::CreateIOSImageDecoder());
  return service;
}

}  // namespace ios_web_view
