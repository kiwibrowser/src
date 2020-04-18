// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/signin_client_factory.h"

#include "base/memory/singleton.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/content_settings/cookie_settings_factory.h"
#include "ios/chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "ios/chrome/browser/signin/ios_chrome_signin_client.h"
#include "ios/chrome/browser/signin/signin_error_controller_factory.h"
#include "ios/chrome/browser/web_data_service_factory.h"

// static
SigninClient* SigninClientFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<SigninClient*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
SigninClientFactory* SigninClientFactory::GetInstance() {
  return base::Singleton<SigninClientFactory>::get();
}

SigninClientFactory::SigninClientFactory()
    : BrowserStateKeyedServiceFactory(
          "SigninClient",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(ios::SigninErrorControllerFactory::GetInstance());
  DependsOn(ios::CookieSettingsFactory::GetInstance());
  DependsOn(ios::HostContentSettingsMapFactory::GetInstance());
  DependsOn(ios::WebDataServiceFactory::GetInstance());
}

SigninClientFactory::~SigninClientFactory() {}

std::unique_ptr<KeyedService> SigninClientFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* chrome_browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  return std::make_unique<IOSChromeSigninClient>(
      chrome_browser_state,
      ios::SigninErrorControllerFactory::GetForBrowserState(
          chrome_browser_state),
      ios::CookieSettingsFactory::GetForBrowserState(chrome_browser_state),
      ios::HostContentSettingsMapFactory::GetForBrowserState(
          chrome_browser_state),
      ios::WebDataServiceFactory::GetTokenWebDataForBrowserState(
          chrome_browser_state, ServiceAccessType::EXPLICIT_ACCESS));
}
