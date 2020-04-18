// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/favicon/ios_chrome_favicon_loader_factory.h"

#include "base/memory/singleton.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/favicon/favicon_loader.h"
#include "ios/chrome/browser/favicon/favicon_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

FaviconLoader* IOSChromeFaviconLoaderFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<FaviconLoader*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

FaviconLoader* IOSChromeFaviconLoaderFactory::GetForBrowserStateIfExists(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<FaviconLoader*>(
      GetInstance()->GetServiceForBrowserState(browser_state, false));
}

IOSChromeFaviconLoaderFactory* IOSChromeFaviconLoaderFactory::GetInstance() {
  return base::Singleton<IOSChromeFaviconLoaderFactory>::get();
}

IOSChromeFaviconLoaderFactory::IOSChromeFaviconLoaderFactory()
    : BrowserStateKeyedServiceFactory(
          "FaviconLoader",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(ios::FaviconServiceFactory::GetInstance());
}

IOSChromeFaviconLoaderFactory::~IOSChromeFaviconLoaderFactory() {}

std::unique_ptr<KeyedService>
IOSChromeFaviconLoaderFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  return std::make_unique<FaviconLoader>(
      ios::FaviconServiceFactory::GetForBrowserState(
          browser_state, ServiceAccessType::IMPLICIT_ACCESS));
}

web::BrowserState* IOSChromeFaviconLoaderFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

bool IOSChromeFaviconLoaderFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
