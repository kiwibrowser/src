// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/google/google_logo_service_factory.h"

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/google/google_logo_service.h"
#include "ios/chrome/browser/search_engines/template_url_service_factory.h"
#include "net/url_request/url_request_context_getter.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// static
GoogleLogoService* GoogleLogoServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<GoogleLogoService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
GoogleLogoServiceFactory* GoogleLogoServiceFactory::GetInstance() {
  return base::Singleton<GoogleLogoServiceFactory>::get();
}

GoogleLogoServiceFactory::GoogleLogoServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "GoogleLogoService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(ios::TemplateURLServiceFactory::GetInstance());
}

GoogleLogoServiceFactory::~GoogleLogoServiceFactory() {}

std::unique_ptr<KeyedService> GoogleLogoServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  return std::make_unique<GoogleLogoService>(
      ios::TemplateURLServiceFactory::GetForBrowserState(browser_state),
      browser_state->GetRequestContext());
}

web::BrowserState* GoogleLogoServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateOwnInstanceInIncognito(context);
}
